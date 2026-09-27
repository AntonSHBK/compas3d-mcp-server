#include "kompas_bridge/transport/named_pipe_server.hpp"

#include <Windows.h>
#include <sddl.h>

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <format>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

#include "kompas_bridge/protocol/codec.hpp"

namespace kompas_bridge {
namespace {

class UniqueHandle {
 public:
  UniqueHandle() = default;
  explicit UniqueHandle(HANDLE handle) : handle_(handle) {}
  ~UniqueHandle() { reset(); }

  UniqueHandle(const UniqueHandle&) = delete;
  UniqueHandle& operator=(const UniqueHandle&) = delete;

  UniqueHandle(UniqueHandle&& other) noexcept
      : handle_(std::exchange(other.handle_, nullptr)) {}

  UniqueHandle& operator=(UniqueHandle&& other) noexcept {
    if (this != &other) {
      reset(std::exchange(other.handle_, nullptr));
    }
    return *this;
  }

  [[nodiscard]] HANDLE get() const noexcept { return handle_; }
  [[nodiscard]] bool valid() const noexcept {
    return handle_ != nullptr && handle_ != INVALID_HANDLE_VALUE;
  }
  void reset(HANDLE handle = nullptr) noexcept {
    if (valid()) {
      CloseHandle(handle_);
    }
    handle_ = handle;
  }

 private:
  HANDLE handle_{};
};

std::runtime_error windows_error(std::string_view operation, DWORD code) {
  return std::runtime_error(
    std::format("{} failed with Windows error {}.", operation, code)
  );
}

std::wstring current_user_sid() {
  HANDLE rawToken{};
  if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &rawToken)) {
    throw windows_error("OpenProcessToken", GetLastError());
  }
  UniqueHandle token(rawToken);

  DWORD size{};
  GetTokenInformation(token.get(), TokenUser, nullptr, 0, &size);
  if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
    throw windows_error("GetTokenInformation(size)", GetLastError());
  }
  std::vector<std::byte> buffer(size);
  if (!GetTokenInformation(
        token.get(),
        TokenUser,
        buffer.data(),
        size,
        &size
      )) {
    throw windows_error("GetTokenInformation", GetLastError());
  }
  const auto* tokenUser = reinterpret_cast<const TOKEN_USER*>(buffer.data());
  LPWSTR rawSid{};
  if (!ConvertSidToStringSidW(tokenUser->User.Sid, &rawSid)) {
    throw windows_error("ConvertSidToStringSidW", GetLastError());
  }
  std::wstring sid(rawSid);
  LocalFree(rawSid);
  return sid;
}

UniqueHandle create_instance_mutex() {
  const std::wstring name = L"Local\\kompas-bridge-" + current_user_sid();
  SetLastError(ERROR_SUCCESS);
  UniqueHandle mutex(CreateMutexW(nullptr, FALSE, name.c_str()));
  if (!mutex.valid()) {
    throw windows_error("CreateMutexW", GetLastError());
  }
  if (GetLastError() == ERROR_ALREADY_EXISTS) {
    throw std::runtime_error(
      "Another kompas_bridge instance is already running for this user."
    );
  }
  return mutex;
}

enum class IoResult { kComplete, kDisconnected, kTimeout };

}  // namespace

class NamedPipeServer::Implementation {
 public:
  Implementation(
    std::wstring pipeName,
    std::chrono::milliseconds requestTimeout
  )
      : pipeName_(std::move(pipeName)),
        requestTimeout_(requestTimeout),
        instanceMutex_(create_instance_mutex()),
        ioEvent_(CreateEventW(nullptr, TRUE, FALSE, nullptr)) {
    constexpr std::wstring_view kPrefix = LR"(\\.\pipe\)";
    if (!pipeName_.starts_with(kPrefix) || pipeName_.size() <= kPrefix.size()) {
      throw std::invalid_argument("Named Pipe path is invalid.");
    }
    if (requestTimeout_.count() <= 0 ||
        requestTimeout_.count() > (std::numeric_limits<DWORD>::max)()) {
      throw std::invalid_argument("Request timeout is invalid.");
    }
    if (!ioEvent_.valid()) {
      throw windows_error("CreateEventW", GetLastError());
    }
    pipe_.reset(CreateNamedPipeW(
      pipeName_.c_str(),
      PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE,
      PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS,
      1,
      static_cast<DWORD>(kMaxMessageBytes + 4),
      static_cast<DWORD>(kMaxMessageBytes + 4),
      static_cast<DWORD>(requestTimeout_.count()),
      nullptr
    ));
    if (!pipe_.valid()) {
      throw windows_error("CreateNamedPipeW", GetLastError());
    }
  }

  bool wait_for_client() {
    if (stopping_) {
      return false;
    }
    if (connected_) {
      throw std::logic_error("A Named Pipe client is already connected.");
    }
    OVERLAPPED operation{};
    reset_event(operation);
    if (ConnectNamedPipe(pipe_.get(), &operation)) {
      connected_ = true;
      return true;
    }
    const DWORD error = GetLastError();
    if (error == ERROR_PIPE_CONNECTED) {
      connected_ = true;
      return true;
    }
    if (error != ERROR_IO_PENDING) {
      throw windows_error("ConnectNamedPipe", error);
    }
    if (WaitForSingleObject(ioEvent_.get(), INFINITE) != WAIT_OBJECT_0) {
      throw windows_error("WaitForSingleObject(connect)", GetLastError());
    }
    DWORD transferred{};
    if (!GetOverlappedResult(pipe_.get(), &operation, &transferred, FALSE)) {
      const DWORD resultError = GetLastError();
      if (stopping_) {
        return false;
      }
      throw windows_error("GetOverlappedResult(connect)", resultError);
    }
    connected_ = true;
    return true;
  }

  void disconnect_client() noexcept {
    if (connected_) {
      CancelIoEx(pipe_.get(), nullptr);
      DisconnectNamedPipe(pipe_.get());
      connected_ = false;
    }
  }

  void request_stop() noexcept {
    stopping_ = true;
    CancelIoEx(pipe_.get(), nullptr);
    SetEvent(ioEvent_.get());
  }

  std::optional<std::string> read_message() {
    require_connected();
    std::array<std::uint8_t, 4> header{};
    if (read_exact(header.data(), header.size()) != IoResult::kComplete) {
      disconnect_client();
      return std::nullopt;
    }
    std::uint32_t size{};
    for (std::size_t index = 0; index < header.size(); ++index) {
      size |= static_cast<std::uint32_t>(header[index]) << (index * 8);
    }
    if (size == 0 || size > kMaxMessageBytes) {
      throw ProtocolError("invalid_request", "Invalid Named Pipe frame length.");
    }
    std::string payload(size, '\0');
    if (read_exact(payload.data(), payload.size()) != IoResult::kComplete) {
      disconnect_client();
      return std::nullopt;
    }
    validate_utf8(payload);
    return payload;
  }

  void write_message(std::string_view message) {
    require_connected();
    const auto frame = encode_frame(message);
    if (write_exact(frame.data(), frame.size()) != IoResult::kComplete) {
      disconnect_client();
      throw std::runtime_error(
        "Named Pipe client disconnected while receiving a response."
      );
    }
  }

  [[nodiscard]] bool is_connected() const noexcept { return connected_; }
  [[nodiscard]] bool is_stopping() const noexcept { return stopping_; }
  [[nodiscard]] std::wstring_view pipe_name() const noexcept { return pipeName_; }

 private:
  void require_connected() const {
    if (!connected_) {
      throw std::logic_error("No Named Pipe client is connected.");
    }
  }

  void reset_event(OVERLAPPED& operation) {
    if (!ResetEvent(ioEvent_.get())) {
      throw windows_error("ResetEvent", GetLastError());
    }
    operation.hEvent = ioEvent_.get();
  }

  IoResult wait_for_io(OVERLAPPED& operation, DWORD& transferred) {
    const DWORD waitResult = WaitForSingleObject(
      ioEvent_.get(),
      static_cast<DWORD>(requestTimeout_.count())
    );
    if (waitResult == WAIT_TIMEOUT) {
      CancelIoEx(pipe_.get(), &operation);
      WaitForSingleObject(ioEvent_.get(), INFINITE);
      return IoResult::kTimeout;
    }
    if (waitResult != WAIT_OBJECT_0) {
      throw windows_error("WaitForSingleObject(I/O)", GetLastError());
    }
    if (GetOverlappedResult(pipe_.get(), &operation, &transferred, FALSE)) {
      return IoResult::kComplete;
    }
    const DWORD error = GetLastError();
    if (error == ERROR_BROKEN_PIPE || error == ERROR_NO_DATA ||
        error == ERROR_PIPE_NOT_CONNECTED || error == ERROR_OPERATION_ABORTED) {
      return IoResult::kDisconnected;
    }
    throw windows_error("GetOverlappedResult(I/O)", error);
  }

  IoResult read_exact(void* destination, std::size_t size) {
    auto* bytes = static_cast<std::byte*>(destination);
    std::size_t offset{};
    while (offset < size) {
      OVERLAPPED operation{};
      reset_event(operation);
      DWORD transferred{};
      if (!ReadFile(
            pipe_.get(),
            bytes + offset,
            static_cast<DWORD>(size - offset),
            &transferred,
            &operation
          )) {
        const DWORD error = GetLastError();
        if (error == ERROR_BROKEN_PIPE || error == ERROR_NO_DATA ||
            error == ERROR_PIPE_NOT_CONNECTED) {
          return IoResult::kDisconnected;
        }
        if (error != ERROR_IO_PENDING) {
          throw windows_error("ReadFile", error);
        }
        const IoResult result = wait_for_io(operation, transferred);
        if (result != IoResult::kComplete) {
          return result;
        }
      }
      if (transferred == 0) {
        return IoResult::kDisconnected;
      }
      offset += transferred;
    }
    return IoResult::kComplete;
  }

  IoResult write_exact(const void* source, std::size_t size) {
    const auto* bytes = static_cast<const std::byte*>(source);
    std::size_t offset{};
    while (offset < size) {
      OVERLAPPED operation{};
      reset_event(operation);
      DWORD transferred{};
      if (!WriteFile(
            pipe_.get(),
            bytes + offset,
            static_cast<DWORD>(size - offset),
            &transferred,
            &operation
          )) {
        const DWORD error = GetLastError();
        if (error == ERROR_BROKEN_PIPE || error == ERROR_NO_DATA ||
            error == ERROR_PIPE_NOT_CONNECTED) {
          return IoResult::kDisconnected;
        }
        if (error != ERROR_IO_PENDING) {
          throw windows_error("WriteFile", error);
        }
        const IoResult result = wait_for_io(operation, transferred);
        if (result != IoResult::kComplete) {
          return result;
        }
      }
      if (transferred == 0) {
        return IoResult::kDisconnected;
      }
      offset += transferred;
    }
    return IoResult::kComplete;
  }

  std::wstring pipeName_;
  std::chrono::milliseconds requestTimeout_;
  UniqueHandle instanceMutex_;
  UniqueHandle ioEvent_;
  UniqueHandle pipe_;
  bool connected_{};
  std::atomic_bool stopping_{};
};

NamedPipeServer::NamedPipeServer(
  std::wstring pipeName,
  std::chrono::milliseconds requestTimeout
)
    : implementation_(std::make_unique<Implementation>(
        std::move(pipeName),
        requestTimeout
      )) {}

NamedPipeServer::~NamedPipeServer() = default;

bool NamedPipeServer::wait_for_client() {
  return implementation_->wait_for_client();
}

void NamedPipeServer::disconnect_client() noexcept {
  implementation_->disconnect_client();
}

void NamedPipeServer::request_stop() noexcept { implementation_->request_stop(); }

bool NamedPipeServer::is_stopping() const noexcept {
  return implementation_->is_stopping();
}

bool NamedPipeServer::is_client_connected() const noexcept {
  return implementation_->is_connected();
}

std::wstring_view NamedPipeServer::pipe_name() const noexcept {
  return implementation_->pipe_name();
}

std::optional<std::string> NamedPipeServer::read_message() {
  return implementation_->read_message();
}

void NamedPipeServer::write_message(std::string_view message) {
  implementation_->write_message(message);
}

}  // namespace kompas_bridge
