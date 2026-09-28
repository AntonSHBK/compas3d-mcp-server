#pragma once
#include <optional>
#include <string>
#include <string_view>
#include <vector>
namespace kompas_bridge {
enum class ExtrusionDirection { kForward, kReverse, kBoth };
enum class BooleanOperation { kNewBody, kJoin, kCut };
struct ExtrusionParameters {
  double distance{};
  ExtrusionDirection direction{ExtrusionDirection::kForward};
  BooleanOperation operation{BooleanOperation::kNewBody};
};
struct ExtrusionResult {
  std::string featureId;
  std::string documentId;
  std::string partId;
  std::string sketchId;
  ExtrusionParameters parameters;
};
struct FeatureInfo {
  std::string featureId;
  std::string documentId;
  std::string partId;
  std::string name;
  std::string featureType;
  bool excluded{};
  bool valid{};
  std::optional<std::string> ownerFeatureId;
  std::uint64_t updateStamp{};
};
class FeatureService {
public:
  virtual ~FeatureService() = default;
  [[nodiscard]] virtual ExtrusionResult
  extrude(std::string_view documentId, std::string_view partId,
          std::string_view sketchId, const ExtrusionParameters &parameters) = 0;
  [[nodiscard]] virtual ExtrusionResult
  get_parameters(std::string_view featureId) = 0;
  [[nodiscard]] virtual ExtrusionResult
  update_extrusion(std::string_view featureId, double distance) = 0;
  [[nodiscard]] virtual std::vector<FeatureInfo>
  list_features(std::string_view partId) = 0;
  [[nodiscard]] virtual FeatureInfo get_info(std::string_view featureId) = 0;
  [[nodiscard]] virtual std::uint64_t rebuild(std::string_view partId) = 0;
};
[[nodiscard]] std::string_view direction_name(ExtrusionDirection value);
[[nodiscard]] std::string_view operation_name(BooleanOperation value);
} // namespace kompas_bridge
