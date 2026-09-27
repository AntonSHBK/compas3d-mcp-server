#pragma once
#include <string>
#include <string_view>
namespace kompas_bridge {
struct Point2d {
  double x{};
  double y{};
};
struct SketchResult {
  std::string sketchId;
  std::string documentId;
  std::string partId;
  std::string plane;
  bool editing{};
  long geometryCount{};
};
class SketchService {
public:
  virtual ~SketchService() = default;
  [[nodiscard]] virtual SketchResult create(std::string_view documentId,
                                            std::string_view partId,
                                            std::string_view plane) = 0;
  [[nodiscard]] virtual SketchResult begin_edit(std::string_view sketchId) = 0;
  [[nodiscard]] virtual SketchResult end_edit(std::string_view sketchId) = 0;
  [[nodiscard]] virtual SketchResult add_line(std::string_view sketchId,
                                              Point2d start, Point2d end) = 0;
  [[nodiscard]] virtual SketchResult
  add_circle(std::string_view sketchId, Point2d center, double radius) = 0;
};
} // namespace kompas_bridge
