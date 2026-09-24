#pragma once

struct IDispatch;
namespace kompas_bridge {
class Api5ApplicationFactory {
 public:
  [[nodiscard]] static IDispatch *CreateApplication();
};
}  // namespace kompas_bridge
