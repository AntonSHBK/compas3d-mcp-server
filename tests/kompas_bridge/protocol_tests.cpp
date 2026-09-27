#include "kompas_bridge/protocol/codec.hpp"
#include "kompas_bridge/protocol/request.hpp"
#include "kompas_bridge/protocol/response.hpp"

#include <stdexcept>
#include <string>

#define CHECK(condition)                                                   \
  do {                                                                     \
    if (!(condition)) {                                                    \
      throw std::runtime_error("Test failed: " #condition);              \
    }                                                                      \
  } while (false)

using namespace kompas_bridge;

int main() {
  const Request request{.id = "req_001", .method = "document.list"};
  const auto requestJson = serialize_request(request);
  const auto parsedRequest = parse_request(requestJson);
  CHECK(parsedRequest.id == request.id);
  CHECK(parsedRequest.method == request.method);
  CHECK(parsedRequest.params.is_object());

  const Response success{.id = request.id,
                         .ok = true,
                         .result = nlohmann::json{{"documents", nlohmann::json::array()}}};
  const auto parsedSuccess = parse_response(serialize_response(success));
  CHECK(parsedSuccess.id == request.id);
  CHECK(parsedSuccess.ok);
  CHECK((*parsedSuccess.result)["documents"].empty());

  const Response failure{.id = request.id,
                         .ok = false,
                         .error = BridgeError{.code = "no_active_document",
                                              .message = "No active document is available."}};
  const auto parsedFailure = parse_response(serialize_response(failure));
  CHECK(!parsedFailure.ok);
  CHECK(parsedFailure.error->code == "no_active_document");
  CHECK(parsedFailure.error->details.is_object());

  const auto frame = encode_frame(requestJson);
  CHECK(frame[0] == static_cast<unsigned char>(requestJson.size()));
  CHECK(decode_frame(frame) == requestJson);

  auto expect_code = [](auto action, const std::string& code) {
    try {
      action();
      throw std::runtime_error("Expected ProtocolError was not thrown.");
    } catch (const ProtocolError& error) {
      CHECK(error.code() == code);
    }
  };
  expect_code([] { (void)parse_request("{"); }, "invalid_request");
  expect_code([] { (void)parse_request(R"({"protocol_version":2,"id":"x","method":"document.list","params":{}})"); },
              "unsupported_protocol_version");
  expect_code([] { (void)parse_request(R"({"protocol_version":1,"id":"x","method":"arbitrary.exec","params":{}})"); },
              "unknown_method");
  expect_code([] { (void)parse_request(R"({"protocol_version":1,"id":"x","method":"document.list","params":[]})"); },
              "invalid_params");
  expect_code([] { (void)decode_frame({}); }, "invalid_request");
  expect_code([] { (void)decode_frame(std::vector<std::uint8_t>{0, 0, 0, 0}); },
              "invalid_request");
  expect_code([] { (void)encode_frame(std::string("\xFF", 1)); }, "invalid_request");
  expect_code([] {
    (void)parse_request(R"({"protocol_version":1,"id":"","method":"document.list","params":{}})");
  }, "invalid_request");
  expect_code([] {
    (void)serialize_response(Response{.id = "x", .ok = false,
      .error = BridgeError{.code = "arbitrary_error", .message = "bad"}});
  }, "invalid_request");
  const Response unidentified{.id = std::nullopt,
                              .ok = false,
                              .error = BridgeError{.code = "invalid_request",
                                                   .message = "Malformed JSON."}};
  CHECK(!parse_response(serialize_response(unidentified)).id);
  expect_code([] { (void)encode_frame(std::string(kMaxMessageBytes + 1, 'x')); },
              "invalid_request");
}
