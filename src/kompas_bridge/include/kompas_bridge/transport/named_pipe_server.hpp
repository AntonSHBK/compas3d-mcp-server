#pragma once

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "kompas_bridge/transport/transport.hpp"

namespace kompas_bridge {

inline constexpr std::wstring_view kDefaultPipeName =
  LR"(\\.\pipe\kompas-bridge)";
inline constexpr std::chrono::milliseconds kDefaultRequestTimeout{30'000};

/** @brief Последовательно обслуживает локальных клиентов Windows Named Pipe. */
class NamedPipeServer final : public Transport {
 public:
  explicit NamedPipeServer(
    std::wstring pipeName = std::wstring(kDefaultPipeName),
    std::chrono::milliseconds requestTimeout = kDefaultRequestTimeout
  );
  ~NamedPipeServer() override;

  NamedPipeServer(const NamedPipeServer&) = delete;
  NamedPipeServer& operator=(const NamedPipeServer&) = delete;
  NamedPipeServer(NamedPipeServer&&) = delete;
  NamedPipeServer& operator=(NamedPipeServer&&) = delete;

  /** @brief Ожидает следующего клиента. */
  [[nodiscard]] bool wait_for_client();
  /** @brief Завершает текущее соединение и сохраняет сервер для нового клиента. */
  void disconnect_client() noexcept;
  /** @brief Прерывает ожидание или I/O для штатного завершения процесса. */
  void request_stop() noexcept;
  [[nodiscard]] bool is_stopping() const noexcept;
  [[nodiscard]] bool is_client_connected() const noexcept;
  [[nodiscard]] std::wstring_view pipe_name() const noexcept;

  [[nodiscard]] std::optional<std::string> read_message() override;
  void write_message(std::string_view message) override;

 private:
  class Implementation;
  std::unique_ptr<Implementation> implementation_;
};

}  // namespace kompas_bridge
