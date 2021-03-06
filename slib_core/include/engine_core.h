#pragma once
#include <atomic>
#include <memory>
#include "core_commands.h"
#include "engine_debug_output.h"
#include "renderer.h"
#include "window.h"

namespace lib_gui {
class TextSystem;
class RectSystem;
}  // namespace lib_gui

namespace lib_graphics {
class Renderer;
class ParticleSystem;
class Window;
class MaterialSystem;
class MeshSystem;
class CullingSystem;
class CameraSystem;
class TransformSystem;
class LightSystem;
}  // namespace lib_graphics

namespace lib_input {
class InputSystem;
}

namespace lib_physics {
class PhysicsSystem;
}

namespace lib_sound {
class SoundSystem;
}

namespace lib_core {
class EngineCore {
 public:
  EngineCore();
  ~EngineCore() = default;

  int StartEngine();
  void SetTimeMultiplier(float multiplier);
  float TimeMultiplier();

  [[nodiscard]] lib_graphics::Renderer* GetRenderer() const;
  [[nodiscard]] lib_physics::PhysicsSystem* GetPhysics() const;
  [[nodiscard]] lib_input::InputSystem* GetInput() const;
  [[nodiscard]] lib_graphics::MaterialSystem* GetMaterial() const;
  [[nodiscard]] lib_gui::TextSystem* GetText() const;
  [[nodiscard]] lib_gui::RectSystem* GetRect() const;
  [[nodiscard]] lib_graphics::MeshSystem* GetMesh() const;
  [[nodiscard]] lib_graphics::Window* GetWindow() const;
  [[nodiscard]] lib_graphics::CullingSystem* GetCulling() const;
  [[nodiscard]] lib_sound::SoundSystem* GetSound() const;
  [[nodiscard]] lib_graphics::TransformSystem* TransformSystem() const;
  [[nodiscard]] lib_graphics::ParticleSystem* ParticleSystem() const;
  [[nodiscard]] lib_graphics::CameraSystem* CameraSystem() const;

  [[nodiscard]] EngineDebugOutput* GetDebugOutput() const;

  static size_t stock_box_mesh, stock_sphere_mesh, stock_material_textured,
      stock_texture, stock_material_untextured;

 private:
  void InitEngine();

  std::unique_ptr<lib_graphics::Window> window_;
  std::unique_ptr<lib_graphics::Renderer> renderer_;
  std::unique_ptr<EngineDebugOutput> debug_output_;

  lib_input::InputSystem* input_system_ = nullptr;
  lib_physics::PhysicsSystem* physics_system_ = nullptr;
  lib_gui::TextSystem* text_system_ = nullptr;
  lib_gui::RectSystem* rect_system_ = nullptr;
  lib_graphics::MeshSystem* mesh_system_ = nullptr;
  lib_graphics::CameraSystem* camera_system_ = nullptr;
  lib_graphics::TransformSystem* transform_system_ = nullptr;
  lib_graphics::MaterialSystem* material_system_ = nullptr;
  lib_graphics::CullingSystem* culling_system_ = nullptr;
  lib_graphics::LightSystem* light_system_ = nullptr;
  lib_sound::SoundSystem* sound_system_ = nullptr;
  lib_graphics::ParticleSystem* particle_system_ = nullptr;

  size_t particle_system_id_;
  size_t input_system_id_;
  size_t physics_system_id_;
  size_t text_system_id_;
  size_t rect_system_id_;
  size_t mesh_system_id_;
  size_t camera_system_id_;
  size_t transform_system_id_;
  size_t material_system_id_;
  size_t culling_system_id_;
  size_t light_system_id_;
  size_t sound_system_id_;

  std::atomic<float> time_multiplier_ = {1.f};
};
}  // namespace lib_core
