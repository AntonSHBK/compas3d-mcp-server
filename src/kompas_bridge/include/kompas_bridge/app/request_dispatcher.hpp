#pragma once

#include <string_view>

#include "kompas_bridge/protocol/response.hpp"

namespace kompas_bridge {
class ApplicationHandler;
class ObjectHandler;
class DocumentHandler;

/** @brief Проверяет JSON-запрос и направляет его нужному обработчику. */
class RequestDispatcher {
 public:
  RequestDispatcher(
    ApplicationHandler& applicationHandler,
    ObjectHandler& objectHandler,
    DocumentHandler& documentHandler
  );
  RequestDispatcher(
    ApplicationHandler& applicationHandler,
    ObjectHandler& objectHandler
  );
  [[nodiscard]] Response dispatch(std::string_view payload) const;

 private:
  ApplicationHandler& applicationHandler_;
  ObjectHandler& objectHandler_;
  DocumentHandler* documentHandler_;
};
}  // namespace kompas_bridge
