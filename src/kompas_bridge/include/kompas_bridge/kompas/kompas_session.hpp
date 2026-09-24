#pragma once

#include <memory>

class KompasObject;
namespace kompas_bridge {
class KompasSession {
 public:
  KompasSession();
  ~KompasSession();
  KompasSession(const KompasSession &) = delete;
  KompasSession &operator=(const KompasSession &) = delete;
  KompasSession(KompasSession &&) = delete;
  KompasSession &operator=(KompasSession &&) = delete;
  [[nodiscard]] static KompasSession Connect();
  [[nodiscard]] bool is_connected() const noexcept;
  [[nodiscard]] KompasObject &kompas_object() const;

 private:
  class Implementation;
  std::unique_ptr<Implementation> implementation_;
};
}  // namespace kompas_bridge
