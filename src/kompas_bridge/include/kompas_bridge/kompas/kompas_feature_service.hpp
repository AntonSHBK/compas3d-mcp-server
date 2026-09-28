#pragma once
#include "kompas_bridge/kompas/feature_service.hpp"
#include <unordered_map>
namespace kompas_bridge {
class ObjectRegistry;
class KompasFeatureService final : public FeatureService {
public:
  explicit KompasFeatureService(ObjectRegistry &registry);
  [[nodiscard]] ExtrusionResult
  extrude(std::string_view documentId, std::string_view partId,
          std::string_view sketchId,
          const ExtrusionParameters &parameters) override;
  [[nodiscard]] ExtrusionResult
  get_parameters(std::string_view featureId) override;
  [[nodiscard]] ExtrusionResult update_extrusion(std::string_view featureId,
                                                 double distance) override;
  [[nodiscard]] std::vector<FeatureInfo>
  list_features(std::string_view partId) override;
  [[nodiscard]] FeatureInfo get_info(std::string_view featureId) override;
  [[nodiscard]] std::uint64_t rebuild(std::string_view partId) override;

private:
  struct Metadata {
    std::string partId;
    std::string sketchId;
    BooleanOperation operation{};
  };
  ObjectRegistry &registry_;
  std::unordered_map<std::string, Metadata> metadata_;
};
} // namespace kompas_bridge
