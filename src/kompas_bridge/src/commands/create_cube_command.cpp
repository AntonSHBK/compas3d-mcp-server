#include "kompas_bridge/commands/create_cube_command.hpp"

#include <WinSock2.h>
#include <afxdisp.h>
#include <kapi5.h>
#include <ksConstants3D.h>

#include <stdexcept>

#include "kompas_bridge/kompas/kompas_session.hpp"

namespace kompas_bridge {

std::string CreateCubeResponse::ToJson() const {
  return R"({"ok":true,"command":"create-cube","size_mm":)" + std::to_string(size_mm) + "}";
}

CreateCubeResponse CreateCube(KompasSession& session, long size_mm) {
  if (!session.is_connected() || size_mm <= 0) {
    throw std::runtime_error("A connected KOMPAS session and positive cube size are required.");
  }

  KompasObject& kompas = session.kompas_object();
  ksDocument3D document(kompas.Document3D());
  if (!document || !document.Create(FALSE, TRUE)) {
    throw std::runtime_error("Failed to create a new 3D document.");
  }

  ksPart part(document.GetPart(pTop_Part));
  ksEntity sketch_entity(part.NewEntity(o3d_sketch));
  ksSketchDefinition sketch_definition(sketch_entity.GetDefinition());
  ksEntity base_plane(part.GetDefaultEntity(o3d_planeXOY));
  if (!part || !sketch_entity || !sketch_definition || !base_plane ||
      !sketch_definition.SetPlane(base_plane) || !sketch_entity.Create()) {
    throw std::runtime_error("Failed to create a sketch on the XOY plane.");
  }

  ksDocument2D sketch(sketch_definition.BeginEdit());
  const double half_size = static_cast<double>(size_mm) / 2.0;
  if (!sketch || !sketch.ksLineSeg(-half_size, -half_size, half_size, -half_size, 1) ||
      !sketch.ksLineSeg(half_size, -half_size, half_size, half_size, 1) ||
      !sketch.ksLineSeg(half_size, half_size, -half_size, half_size, 1) ||
      !sketch.ksLineSeg(-half_size, half_size, -half_size, -half_size, 1) ||
      !sketch_definition.EndEdit()) {
    throw std::runtime_error("Failed to draw the cube base sketch.");
  }

  ksEntity extrusion_entity(part.NewEntity(o3d_bossExtrusion));
  ksBossExtrusionDefinition extrusion_definition(extrusion_entity.GetDefinition());
  if (!extrusion_entity || !extrusion_definition ||
      !extrusion_definition.SetSideParam(TRUE, etBlind, size_mm, 0, FALSE) ||
      !extrusion_definition.SetSketch(sketch_entity) || !extrusion_entity.Create()) {
    throw std::runtime_error("Failed to create the cube extrusion.");
  }
  return CreateCubeResponse{.size_mm = size_mm};
}

}  // namespace kompas_bridge
