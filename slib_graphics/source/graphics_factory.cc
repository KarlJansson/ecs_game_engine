#include "graphics_factory.h"
#include "gl_camera_system.h"
#include "gl_deferred_renderer.h"
#include "gl_material_system.h"
#include "gl_mesh_system.h"
#include "gl_particle_system.h"
#include "gl_renderer.h"
#include "gl_text_system.h"
#include "gl_window.h"

namespace lib_graphics {
std::unique_ptr<Window> GraphicsFactory::CreateAppWindow() {
  return std::make_unique<GlWindow>();
}

std::unique_ptr<Renderer> GraphicsFactory::CreateForwardRenderer(
    lib_core::EngineCore *engine) {
  return std::make_unique<GlRenderer>(engine);
}

std::unique_ptr<Renderer> GraphicsFactory::CreateDeferredRenderer(
    lib_core::EngineCore *engine) {
  return std::make_unique<GlDeferredRenderer>(engine);
}

std::unique_ptr<CameraSystem> GraphicsFactory::CreateCameraSystem() {
  return std::make_unique<GlCameraSystem>();
}

std::unique_ptr<MeshSystem> GraphicsFactory::CreateMeshSystem(
    lib_core::EngineCore *engine) {
  auto ptr = std::make_unique<GlMeshSystem>(engine);
  ptr->StartLoadThread();
  return ptr;
}

std::unique_ptr<TransformSystem> GraphicsFactory::CreateTransformSystem() {
  return std::make_unique<TransformSystem>();
}

std::unique_ptr<MaterialSystem> GraphicsFactory::CreateMaterialSystem(
    lib_core::EngineCore *engine) {
  auto ptr = std::make_unique<GlMaterialSystem>(engine);
  ptr->StartLoadThread();
  return ptr;
}

std::unique_ptr<CullingSystem> GraphicsFactory::CreateCullingSystem(
    lib_core::EngineCore *engine) {
  return std::make_unique<CullingSystem>(engine);
}

std::unique_ptr<LightSystem> GraphicsFactory::CreateLightSystem() {
  return std::make_unique<LightSystem>();
}

std::unique_ptr<ParticleSystem> GraphicsFactory::CreateParticleSystem(
    const lib_core::EngineCore *engine) {
  return std::make_unique<GlParticleSystem>(engine);
}
}  // namespace lib_graphics
