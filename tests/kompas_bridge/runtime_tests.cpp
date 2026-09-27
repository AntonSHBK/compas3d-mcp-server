#include "kompas_bridge/app/request_dispatcher.hpp"
#include "kompas_bridge/app/request_loop.hpp"
#include "kompas_bridge/handlers/application_handler.hpp"
#include "kompas_bridge/handlers/object_handler.hpp"
#include "kompas_bridge/kompas/application_service.hpp"
#include "kompas_bridge/kompas/object_registry.hpp"
#include "kompas_bridge/protocol/response.hpp"
#include "kompas_bridge/transport/transport.hpp"

#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

void check(bool condition) {
  if (!condition) {
    throw std::runtime_error("Runtime test failed.");
  }
}

class FakeTransport final : public kompas_bridge::Transport {
 public:
  explicit FakeTransport(std::vector<std::string> requests)
      : requests_(std::move(requests)) {}

  std::optional<std::string> read_message() override {
    if (nextRequest_ == requests_.size()) {
      return std::nullopt;
    }
    return requests_[nextRequest_++];
  }

  void write_message(std::string_view message) override {
    responses_.emplace_back(message);
  }

  [[nodiscard]] const std::vector<std::string>& responses() const {
    return responses_;
  }

 private:
  std::vector<std::string> requests_;
  std::vector<std::string> responses_;
  std::size_t nextRequest_{};
};

class FakeApplicationService final : public kompas_bridge::ApplicationService {
 public:
  kompas_bridge::ApplicationStatus get_status() override {
    ++statusCalls;
    if (!connected) {
      return {};
    }
    return {.connected = true,
            .visible = true,
            .kompasVersion = kompas_bridge::KompasVersion{
                .major = 24, .minor = 0, .release = 7, .build = 0}};
  }

  kompas_bridge::ApplicationStatus connect(
      kompas_bridge::ConnectionPolicy policy) override {
    ++connectCalls;
    lastPolicy = policy;
    connected = true;
    return get_status();
  }

  void disconnect() override {
    ++disconnectCalls;
    connected = false;
  }

  bool connected{};
  int statusCalls{};
  int connectCalls{};
  int disconnectCalls{};
  kompas_bridge::ConnectionPolicy lastPolicy{};
};

}  // namespace

int main() {
  FakeApplicationService service;
  kompas_bridge::ApplicationHandler handler(service);
  kompas_bridge::ObjectRegistry registry;
  kompas_bridge::ObjectHandler objectHandler(registry);
  kompas_bridge::RequestDispatcher dispatcher(handler, objectHandler);
  FakeTransport transport({
      R"({"protocol_version":1,"id":"one","method":"application.status","params":{}})",
      R"({"protocol_version":1,"id":"connect","method":"application.connect","params":{}})",
      R"({"protocol_version":1,"id":"two","method":"application.status","params":{}})",
      R"({"protocol_version":1,"id":"bad","method":"application.status","params":{"x":1}})",
      R"({"protocol_version":1,"id":"disconnect","method":"application.disconnect","params":{}})",
      R"({"protocol_version":1,"id":"third","method":"application.status","params":{}})",
      R"({"protocol_version":1,"id":"start","method":"application.connect","params":{"policy":"start_new"}})",
      R"({"protocol_version":1,"id":"attach_or_start","method":"application.connect","params":{"policy":"attach_or_start"}})",
      R"({"protocol_version":1,"id":"invalid_policy","method":"application.connect","params":{"policy":"other"}})"});

  kompas_bridge::run_request_loop(transport, dispatcher);

  const auto& responses = transport.responses();
  check(responses.size() == 9);
  const auto first = kompas_bridge::parse_response(responses[0]);
  const auto initialConnect = kompas_bridge::parse_response(responses[1]);
  const auto second = kompas_bridge::parse_response(responses[2]);
  const auto invalid = kompas_bridge::parse_response(responses[3]);
  const auto disconnected = kompas_bridge::parse_response(responses[4]);
  const auto third = kompas_bridge::parse_response(responses[5]);
  const auto started = kompas_bridge::parse_response(responses[6]);
  const auto attachOrStart = kompas_bridge::parse_response(responses[7]);
  const auto invalidPolicy = kompas_bridge::parse_response(responses[8]);
  check(first.ok && first.id == "one" && (*first.result)["connected"] == false);
  check((*first.result)["visible"].is_null());
  check(initialConnect.ok && (*initialConnect.result)["connected"] == true);
  check(second.ok && second.id == "two" && (*second.result)["connected"] == true);
  check((*second.result)["kompas_version"]["major"] == 24);
  check(!invalid.ok && invalid.id == "bad" &&
        invalid.error->code == "invalid_params");
  check(disconnected.ok && (*disconnected.result)["connected"] == false);
  check(third.ok && third.id == "third" && (*third.result)["connected"] == false);
  check(started.ok && (*started.result)["connected"] == true);
  check(attachOrStart.ok && (*attachOrStart.result)["connected"] == true);
  check(!invalidPolicy.ok && invalidPolicy.error->code == "invalid_params");
  check(service.connectCalls == 3);
  check(service.disconnectCalls == 1);
  check(service.lastPolicy ==
        kompas_bridge::ConnectionPolicy::kAttachOrStart);
}
