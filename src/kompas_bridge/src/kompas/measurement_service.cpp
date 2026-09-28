#include <WinSock2.h>
#include <afxdisp.h>
#include <kapi5.h>

#include <functional>
#include <utility>

#include "kompas_bridge/kompas/kompas_measurement_service.hpp"
#include "kompas_bridge/kompas/object_registry.hpp"
#include "kompas_bridge/protocol/codec.hpp"

namespace kompas_bridge {
namespace {

constexpr unsigned long kMillimeterUnit = 1;

struct MeasurementContext {
  ksMeasurer measurer;
};

Microsoft::WRL::ComPtr<IDispatch> dispatch_for(ObjectRegistry& registry,
                                               std::string_view id,
                                               ObjectKind kind) {
  auto object = registry.get_object(id, kind);
  Microsoft::WRL::ComPtr<IDispatch> dispatch;
  if (FAILED(object.As(&dispatch))) {
    throw ProtocolError("kompas_api_error",
                        "Measured object has no IDispatch interface.");
  }
  return dispatch;
}

template <typename Function>
auto cad_call(Function&& function) -> decltype(function()) {
  try {
    return function();
  } catch (CException* error) {
    error->Delete();
    throw ProtocolError("kompas_api_error",
                        "KOMPAS measurement operation failed.");
  }
}

MeasurementContext create_measurer(ObjectRegistry& registry,
                                   std::string_view object1Id,
                                   std::string_view object2Id) {
  const auto info1 = registry.get_info(object1Id);
  const auto info2 = registry.get_info(object2Id);
  if (!info1.partId || !info2.partId || *info1.partId != *info2.partId) {
    throw ProtocolError("invalid_params",
                        "Measured objects must belong to the same part.");
  }
  auto partDispatch = dispatch_for(registry, *info1.partId, ObjectKind::kPart);
  auto object1 = dispatch_for(registry, object1Id, ObjectKind::kFace);
  auto object2 = dispatch_for(registry, object2Id, ObjectKind::kFace);
  ksPart part(partDispatch.Detach());
  ksMeasurer measurer(part.GetMeasurer());
  if (measurer.m_lpDispatch == nullptr) {
    throw ProtocolError("kompas_api_error",
                        "KOMPAS did not create a measurer.");
  }
  measurer.SetUnit(kMillimeterUnit);
  if (!measurer.SetObject1(object1.Get()) ||
      !measurer.SetObject2(object2.Get()) || !measurer.Calc()) {
    throw ProtocolError("kompas_api_error",
                        "KOMPAS could not measure the selected objects.");
  }
  return {std::move(measurer)};
}

std::optional<Point3D> read_point(
    const std::function<BOOL(double*, double*, double*)>& getter) {
  Point3D point;
  if (!getter(&point.x, &point.y, &point.z)) {
    return std::nullopt;
  }
  return point;
}

}  // namespace

KompasMeasurementService::KompasMeasurementService(ObjectRegistry& registry)
    : registry_(registry) {}

DistanceMeasurement KompasMeasurementService::distance(
    std::string_view object1Id, std::string_view object2Id) {
  return cad_call([&] {
    auto context = create_measurer(registry_, object1Id, object2Id);
    auto& measurer = context.measurer;
    const auto point1 = read_point([&](double* x, double* y, double* z) {
      return measurer.GetPoint1(x, y, z);
    });
    const auto point2 = read_point([&](double* x, double* y, double* z) {
      return measurer.GetPoint2(x, y, z);
    });
    if (!point1 || !point2) {
      throw ProtocolError("kompas_api_error",
                          "KOMPAS did not return minimum-distance points.");
    }
    const auto maxPoint1 = read_point([&](double* x, double* y, double* z) {
      return measurer.GetMaxPoint1(x, y, z);
    });
    const auto maxPoint2 = read_point([&](double* x, double* y, double* z) {
      return measurer.GetMaxPoint2(x, y, z);
    });
    const auto normalPoint1 =
        read_point([&](double* x, double* y, double* z) {
          return measurer.GetNormalPoint1(x, y, z);
        });
    const auto normalPoint2 =
        read_point([&](double* x, double* y, double* z) {
          return measurer.GetNormalPoint2(x, y, z);
        });
    return DistanceMeasurement{
        .distanceMm = measurer.GetDistance(),
        .point1 = *point1,
        .point2 = *point2,
        .maximumDistanceMm = maxPoint1 && maxPoint2
                                 ? std::optional(measurer.GetMaxDistance())
                                 : std::nullopt,
        .maximumPoint1 = maxPoint1,
        .maximumPoint2 = maxPoint2,
        .normalDistanceMm = normalPoint1 && normalPoint2
                                ? std::optional(measurer.GetNormalDistance())
                                : std::nullopt,
        .normalPoint1 = normalPoint1,
        .normalPoint2 = normalPoint2};
  });
}

double KompasMeasurementService::angle_degrees(std::string_view object1Id,
                                                std::string_view object2Id) {
  return cad_call([&] {
    auto context = create_measurer(registry_, object1Id, object2Id);
    if (!context.measurer.IsAngleValid()) {
      throw ProtocolError("measurement_not_available",
                          "Angle is not defined for the selected objects.");
    }
    return context.measurer.GetAngle();
  });
}

}  // namespace kompas_bridge
