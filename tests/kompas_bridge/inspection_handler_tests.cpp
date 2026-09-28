#include "kompas_bridge/handlers/inspection_handler.hpp"

#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "kompas_bridge/kompas/inspection_service.hpp"

namespace {

void check(bool condition) {
  if (!condition) {
    throw std::runtime_error("Inspection handler test failed.");
  }
}

class FakeInspectionService final : public kompas_bridge::InspectionService {
 public:
  std::vector<kompas_bridge::BodyInfo> list_bodies(
    std::string_view partId
  ) override {
    return {{
      .bodyId = "body_1",
      .partId = std::string(partId),
      .solid = true,
      .boundingBox = {{-1, -1, 0}, {1, 1, 2}},
      .ownerFeatureId = "feat_1"
    }};
  }

  kompas_bridge::BodyInfo get_body_info(std::string_view bodyId) override {
    return {
      .bodyId = std::string(bodyId),
      .partId = "part_1",
      .solid = true,
      .boundingBox = {{-1, -1, 0}, {1, 1, 2}}
    };
  }

  std::vector<kompas_bridge::FaceInfo> list_body_faces(
    std::string_view bodyId
  ) override {
    return {face(std::string(bodyId))};
  }

  std::vector<kompas_bridge::FaceInfo> list_part_faces(
    std::string_view
  ) override {
    return {face("body_1")};
  }

  kompas_bridge::FaceInfo get_face_geometry(std::string_view) override {
    return face("body_1");
  }

  kompas_bridge::MassProperties get_mass_properties(
    std::string_view
  ) override {
    return {
      .massKg = 1.5,
      .volumeMm3 = 200.0,
      .areaMm2 = 300.0,
      .densityKgM3 = 7800.0,
      .centerOfMass = {0, 0, 1},
      .moments = {1, 2, 3, 0, 0, 0}
    };
  }

  kompas_bridge::BoundingBox3D get_bounding_box(
    std::string_view
  ) override {
    return {{-1, -2, -3}, {4, 5, 6}};
  }

 private:
  static kompas_bridge::FaceInfo face(std::string bodyId) {
    return {
      .faceId = "face_1",
      .partId = "part_1",
      .bodyId = std::move(bodyId),
      .surfaceType = "plane",
      .areaMm2 = 4.0,
      .normal = kompas_bridge::Point3D{0, 0, 1},
      .boundingBox = {{-1, -1, 2}, {1, 1, 2}},
      .ownerFeatureId = "feat_1"
    };
  }
};

}  // namespace

int main() {
  FakeInspectionService service;
  kompas_bridge::InspectionHandler handler(service);

  const auto bodies = handler.list_bodies({{"part_id", "part_1"}});
  check(bodies["bodies"][0]["body_id"] == "body_1");
  check(bodies["bodies"][0]["solid"] == true);

  const auto faces = handler.list_body_faces({{"body_id", "body_1"}});
  check(faces["faces"][0]["surface_type"] == "plane");
  check(faces["faces"][0]["normal"]["z"] == 1.0);

  const auto mass = handler.get_mass_properties({{"part_id", "part_1"}});
  check(mass["mass_kg"] == 1.5);
  check(mass["moments_of_inertia"]["jz"] == 3.0);

  const auto box = handler.get_bounding_box({{"part_id", "part_1"}});
  check(box["min"]["y"] == -2.0);
  check(box["max"]["z"] == 6.0);
}
