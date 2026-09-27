#pragma once
#include "kompas_bridge/kompas/sketch_service.hpp"
#include <string>
#include <unordered_map>
#include <wrl/client.h>
struct IDispatch;
namespace kompas_bridge {
class ObjectRegistry;
class KompasSketchService final : public SketchService {
public:
  explicit KompasSketchService(ObjectRegistry &registry);
  [[nodiscard]] SketchResult create(std::string_view documentId,
                                    std::string_view partId,
                                    std::string_view plane) override;
  [[nodiscard]] SketchResult begin_edit(std::string_view sketchId) override;
  [[nodiscard]] SketchResult end_edit(std::string_view sketchId) override;
  [[nodiscard]] SketchResult add_line(std::string_view sketchId, Point2d start,
                                      Point2d end) override;
  [[nodiscard]] SketchResult add_circle(std::string_view sketchId,
                                        Point2d center, double radius) override;

private:
  [[nodiscard]] IDispatch *edit_document(std::string_view sketchId) const;
  [[nodiscard]] SketchResult result(std::string_view sketchId,
                                    bool editing) const;
  ObjectRegistry &registry_;
  std::unordered_map<std::string, Microsoft::WRL::ComPtr<IDispatch>> edits_;
  struct Metadata {
    std::string documentId;
    std::string partId;
    std::string plane;
    long geometryCount{};
  };
  std::unordered_map<std::string, Metadata> metadata_;
};
} // namespace kompas_bridge
