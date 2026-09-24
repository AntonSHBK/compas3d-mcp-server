#pragma once

#include <string>

namespace kompas_bridge {
class KompasSession;

struct CreateCubeResponse {
  long size_mm{};
  [[nodiscard]] std::string ToJson() const;
};

[[nodiscard]] CreateCubeResponse CreateCube(KompasSession& session, long size_mm = 50);
}  // namespace kompas_bridge
