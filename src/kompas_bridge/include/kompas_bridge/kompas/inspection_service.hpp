#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace kompas_bridge {

struct Point3D {
  double x{};
  double y{};
  double z{};
};

struct BoundingBox3D {
  Point3D min;
  Point3D max;
};

struct BodyInfo {
  std::string bodyId;
  std::string partId;
  bool solid{};
  BoundingBox3D boundingBox;
  std::optional<std::string> ownerFeatureId;
};

struct FaceInfo {
  std::string faceId;
  std::string partId;
  std::optional<std::string> bodyId;
  std::string surfaceType;
  double areaMm2{};
  std::optional<Point3D> normal;
  BoundingBox3D boundingBox;
  std::optional<double> radiusMm;
  std::optional<std::string> ownerFeatureId;
};

struct MomentsOfInertia {
  double jx{};
  double jy{};
  double jz{};
  double jxy{};
  double jxz{};
  double jyz{};
};

struct MassProperties {
  double massKg{};
  double volumeMm3{};
  double areaMm2{};
  double densityKgM3{};
  Point3D centerOfMass;
  MomentsOfInertia moments;
};

class InspectionService {
 public:
  virtual ~InspectionService() = default;
  [[nodiscard]] virtual std::vector<BodyInfo> list_bodies(
      std::string_view partId) = 0;
  [[nodiscard]] virtual BodyInfo get_body_info(std::string_view bodyId) = 0;
  [[nodiscard]] virtual std::vector<FaceInfo> list_body_faces(
      std::string_view bodyId) = 0;
  [[nodiscard]] virtual std::vector<FaceInfo> list_part_faces(
      std::string_view partId) = 0;
  [[nodiscard]] virtual FaceInfo get_face_geometry(std::string_view faceId) = 0;
  [[nodiscard]] virtual MassProperties get_mass_properties(
      std::string_view partId) = 0;
  [[nodiscard]] virtual BoundingBox3D get_bounding_box(
      std::string_view partId) = 0;
};

}  // namespace kompas_bridge
