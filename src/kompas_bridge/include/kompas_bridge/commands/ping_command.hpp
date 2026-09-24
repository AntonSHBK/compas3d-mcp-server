#pragma once

#include <string>

namespace kompas_bridge {
class KompasSession;
struct KompasVersion {
  long major{};
  long minor{};
  long release{};
  long build{};
};
struct PingResponse {
  KompasVersion kompas_version;
  [[nodiscard]] std::string ToJson() const;
};
[[nodiscard]] PingResponse RunPing(KompasSession &session);
}  // namespace kompas_bridge
