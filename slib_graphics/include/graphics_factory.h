#pragma once
#include "component/camera.h"
#include "component/mesh.h"
#include "engine_core.h"
#include "renderer.h"
#include "system.h"
#include "system/culling_system.h"
#include "system/light_system.h"
#include "system/transform_system.h"
#include "window.h"

namespace lib_graphics {
class GraphicsFactory {
public:
  GraphicsFactory() = default;
  ~GraphicsFactory() = default;

  std::unique_ptr<Window> CreateAppWindow();
  std::unique_ptr<Renderer> CreateForwardRenderer(lib_core::EngineCore *engine);
  std::unique_ptr<Renderer>
  CreateDeferredRenderer(lib_core::EngineCore *engine);

  std::unique_ptr<CameraSystem> CreateCameraSystem();
  std::unique_ptr<MeshSystem> CreateMeshSystem(lib_core::EngineCore *engine);
  std::unique_ptr<TransformSystem> CreateTransformSystem();
  std::unique_ptr<MaterialSystem>
  CreateMaterialSystem(lib_core::EngineCore *engine);
  std::unique_ptr<CullingSystem>
  CreateCullingSystem(lib_core::EngineCore *engine);
  std::unique_ptr<LightSystem> CreateLightSystem();
  std::unique_ptr<ParticleSystem>
  CreateParticleSystem(const lib_core::EngineCore *engine);
};
} // namespace lib_graphics
