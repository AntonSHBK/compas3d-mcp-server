#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>

#include <nlohmann/json.hpp>

#include "kompas_bridge/protocol/response.hpp"

namespace kompas_bridge {

/** @brief Routes validated protocol requests to registered callbacks. */
class RequestDispatcher {
 public:
  using Route =
    std::function<nlohmann::json(const nlohmann::json& params)>;

  /** @brief Registers one unique protocol method. */
  void add_route(std::string_view method, Route route);

  [[nodiscard]] Response dispatch(std::string_view payload) const;

 private:
  std::unordered_map<std::string, Route> routes_;
};

}  // namespace kompas_bridge
