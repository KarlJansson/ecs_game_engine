#include "engine_core.h"
#include "core_commands.h"
#include "engine_settings.h"
#include "entity_manager.h"
#include "graphics_factory.h"
#include "gui_factory.h"
#include "gui_text.h"
#include "input_factory.h"
#include "input_system.h"
#include "key_definitions.h"
#include "physics_factory.h"
#include "physics_system.h"
#include "renderer.h"
#include "sound_factory.h"
#include "system/camera_system.h"
#include "system/culling_system.h"
#include "system/material_system.h"
#include "system/mesh_system.h"
#include "system/particle_system.h"
#include "system/transform_system.h"
#include "system_manager.h"
#include "text_system.h"
#include "window.h"

namespace lib_core {
size_t EngineCore::stock_box_mesh, EngineCore::stock_sphere_mesh,
    EngineCore::stock_material_untextured, EngineCore::stock_material_textured,
    EngineCore::stock_texture;

EngineCore::EngineCore() { InitEngine(); }

int EngineCore::StartEngine() {
  auto gfx_mgr = lib_graphics::GraphicsFactory();

  std::atomic<int> fps = {0};
  std::atomic<bool> restart = {false};
  std::atomic<bool> rebuild_rec = {false};
  std::atomic<bool> run = {true};
  std::atomic<float> frame_time = {.0f}, max_frame_time = {.0f};

  auto render_thread = [&]() {
    window_->SetRenderContext();
    renderer_ = gfx_mgr.CreateDeferredRenderer(this);
    renderer_->InitRenderer();
    if (rebuild_rec) {
      mesh_system_->RebuildResources();
      particle_system_->RebuildGpuResources();
      material_system_->RebuildTextures();
    }

    float reset_timer = 1.f;
    int fps_count = 0;
    float fps_freq = .0f;

    float dt = .0f;
    std::chrono::duration<float> elapsed;
    std::chrono::duration<float, std::milli> elapsed_milli;
    std::chrono::time_point<std::chrono::high_resolution_clock> start_point;

    float sleep;
    float sleep_time = g_settings.FramePace();
    float fps_target = g_settings.FpsTarget() * .5f;
    while (run) {
      start_point = std::chrono::high_resolution_clock::now();

      g_ent_mgr.DrawUpdate();
      g_sys_mgr.DrawUpdate(renderer_.get(), GetText());

      renderer_->RenderFrame(dt * time_multiplier_);
      reset_timer -= dt;
      if (reset_timer < 0.f) {
        reset_timer += 1.f;
        max_frame_time = 0.f;
      }

      g_ent_mgr.FrameFinished();
      window_->SwapBuffers();
      elapsed_milli = std::chrono::high_resolution_clock::now() - start_point;
      if (max_frame_time < elapsed_milli.count())
        max_frame_time = elapsed_milli.count();
      frame_time = elapsed_milli.count();

      elapsed = std::chrono::high_resolution_clock::now() - start_point;
      sleep = sleep_time - elapsed.count();
      if (sleep > 0.f)
        std::this_thread::sleep_for(std::chrono::duration<float>(sleep));

      elapsed = std::chrono::high_resolution_clock::now() - start_point;
      dt = elapsed.count();

      ++fps_count;
      fps_freq += dt;
      if (fps_freq > 0.5f) {
        if (fps_count < fps_target && sleep_time > 0.f)
          sleep_time -= g_settings.FramePace() * .01f;
        else if (fps_count > fps_target)
          sleep_time += g_settings.FramePace() * .01f;

        if (fps_target != g_settings.FpsTarget() * .5f) {
          fps_target = g_settings.FpsTarget() * .5f;
          sleep_time = g_settings.FramePace();
        }

        fps = fps_count;
        fps_count = 0;
        fps_freq -= .5f;
      }
    }

    mesh_system_->PurgeGpuResources();
    text_system_->PurgeGpuResources();
    rect_system_->PurgeGpuResources();
    material_system_->PurgeGpuResources();
    particle_system_->PurgeGpuResources();

    if (!restart) {
      issue_command(lib_core::RemoveSystemCommand(camera_system_id_));
      issue_command(lib_core::RemoveSystemCommand(material_system_id_));
      issue_command(lib_core::RemoveSystemCommand(input_system_id_));
      issue_command(lib_core::RemoveSystemCommand(culling_system_id_));
      issue_command(lib_core::RemoveSystemCommand(transform_system_id_));
      issue_command(lib_core::RemoveSystemCommand(mesh_system_id_));
      issue_command(lib_core::RemoveSystemCommand(text_system_id_));
      issue_command(lib_core::RemoveSystemCommand(rect_system_id_));
      issue_command(lib_core::RemoveSystemCommand(particle_system_id_));
    }
    renderer_.reset();
  };

  auto update_thread = [&]() {
    std::chrono::duration<float> elapsed;
    std::chrono::duration<float, std::milli> elapsed_milli;
    std::chrono::time_point<std::chrono::high_resolution_clock> start_point;

    int ups = 0;
    float ups_freq = .0f;
    float dt = 0.0;
    float max_time = .0f;
    float reset_timer = 100.f;
    bool toggle_pressed = false;
    ct::string max_time_str, ups_str;
    ct::string max_frame_str, fps_str;

    while (!window_->ShouldClose() && !restart) {
      start_point = std::chrono::high_resolution_clock::now();
      g_sys_mgr.LogicUpdate(dt * time_multiplier_);

      if (input_system_->KeyPressed(lib_input::kLeftAlt)) {
        if (!toggle_pressed && input_system_->KeyPressed(lib_input::k1)) {
          debug_output_->ToggleDebugOutput();
          toggle_pressed = true;
        } else if (!toggle_pressed &&
                   input_system_->KeyPressed(lib_input::k2)) {
          debug_output_->ToggleTopLeftDebugOutput();
          toggle_pressed = true;
        } else if (!toggle_pressed &&
                   input_system_->KeyPressed(lib_input::k3)) {
          debug_output_->ToggleTopRightDebugOutput();
          toggle_pressed = true;
        } else if (!toggle_pressed &&
                   input_system_->KeyPressed(lib_input::k4)) {
          debug_output_->ToggleBottomLeftDebugOutput();
          toggle_pressed = true;
        } else if (!toggle_pressed &&
                   input_system_->KeyPressed(lib_input::k5)) {
          debug_output_->ToggleBottomRightDebugOutput();
          toggle_pressed = true;
        } else if (!toggle_pressed &&
                   input_system_->KeyPressed(lib_input::kZ)) {
          g_settings.SetSsao(!g_settings.Ssao());
          toggle_pressed = true;
        } else if (!toggle_pressed &&
                   input_system_->KeyPressed(lib_input::kX)) {
          g_settings.SetBloom(!g_settings.Bloom());
          toggle_pressed = true;
        } else if (!toggle_pressed &&
                   input_system_->KeyPressed(lib_input::kC)) {
          g_settings.SetSmaa(!g_settings.Smaa());
          toggle_pressed = true;
        }

        if (input_system_->KeyReleased(lib_input::k1) &&
            input_system_->KeyReleased(lib_input::k2) &&
            input_system_->KeyReleased(lib_input::k3) &&
            input_system_->KeyReleased(lib_input::k4) &&
            input_system_->KeyReleased(lib_input::k5) &&
            input_system_->KeyReleased(lib_input::kZ) &&
            input_system_->KeyReleased(lib_input::kX) &&
            input_system_->KeyReleased(lib_input::kC))
          toggle_pressed = false;
      }

      ++ups;
      ups_freq += dt;
      if (ups_freq > 0.5f) {
        debug_output_->UpdateTopLeftLine(0, "ups: " + std::to_string(2 * ups));
        debug_output_->UpdateTopLeftLine(1, "fps: " + std::to_string(2 * fps));
        ups_freq = 0.0;
        ups = 0;
      }

      elapsed_milli = std::chrono::high_resolution_clock::now() - start_point;
      reset_timer -= elapsed_milli.count();
      if (reset_timer < 0.f) {
        reset_timer += 100.f;
        max_time = 0.f;
      }

      if (max_time < elapsed_milli.count()) max_time = elapsed_milli.count();
      max_time_str = std::to_string(max_time);
      ups_str = std::to_string(elapsed_milli.count());
      debug_output_->UpdateTopRightLine(
          0, ups_str.substr(0, ups_str.find_first_of('.')) + "ms : " +
                 max_time_str.substr(0, max_time_str.find_first_of('.')) +
                 "ms :Update time:");
      max_frame_str = std::to_string(max_frame_time);
      fps_str = std::to_string(frame_time);
      debug_output_->UpdateTopRightLine(
          1, fps_str.substr(0, fps_str.find_first_of('.')) + "ms : " +
                 max_frame_str.substr(0, max_frame_str.find_first_of('.')) +
                 "ms :Frame time:");

      g_ent_mgr.LogicUpdate();
      elapsed = std::chrono::high_resolution_clock::now() - start_point;
      dt = elapsed.count();

      restart = window_->NeedsRestart();
    }

    g_ent_mgr.FrameFinished();

    run = false;
  };

  do {
    run = true;
    restart = false;

    debug_output_->GenerateFont(int(window_->GetWindowDim().second * .03f));
    std::thread render(render_thread);
    update_thread();
    render.join();

    if (restart) {
      g_ent_mgr.ResetSync();
      g_sys_mgr.CleanSystems();

      renderer_.reset();
      window_->Rebuild();
      input_system_->InitSystem();

      renderer_ = gfx_mgr.CreateDeferredRenderer(this);
    }
    rebuild_rec = true;
  } while (restart);

  input_system_ = nullptr;
  physics_system_ = nullptr;
  text_system_ = nullptr;
  rect_system_ = nullptr;
  mesh_system_ = nullptr;
  camera_system_ = nullptr;
  transform_system_ = nullptr;
  material_system_ = nullptr;
  culling_system_ = nullptr;
  light_system_ = nullptr;
  sound_system_ = nullptr;
  particle_system_ = nullptr;
  g_sys_mgr.ClearSystems();

  cu::PrintLogFile("./logfile.txt");
  cu::PrintProfiling("./profiling.txt");
  return 1;
}

void EngineCore::SetTimeMultiplier(float multiplier) {
  time_multiplier_ = multiplier;
}

float EngineCore::TimeMultiplier() { return time_multiplier_; }

lib_graphics::Renderer *EngineCore::GetRenderer() const {
  return renderer_.get();
}

lib_physics::PhysicsSystem *EngineCore::GetPhysics() const {
  return physics_system_;
}

lib_input::InputSystem *EngineCore::GetInput() const { return input_system_; }

lib_graphics::MaterialSystem *EngineCore::GetMaterial() const {
  return material_system_;
}

lib_gui::TextSystem *EngineCore::GetText() const { return text_system_; }

lib_gui::RectSystem *EngineCore::GetRect() const { return rect_system_; }

lib_graphics::MeshSystem *EngineCore::GetMesh() const { return mesh_system_; }

lib_graphics::Window *EngineCore::GetWindow() const { return window_.get(); }

lib_graphics::CullingSystem *EngineCore::GetCulling() const {
  return culling_system_;
}

lib_sound::SoundSystem *EngineCore::GetSound() const { return sound_system_; }

lib_graphics::TransformSystem *EngineCore::TransformSystem() const {
  return transform_system_;
}

lib_graphics::ParticleSystem *EngineCore::ParticleSystem() const {
  return particle_system_;
}

lib_graphics::CameraSystem *EngineCore::CameraSystem() const {
  return camera_system_;
}

EngineDebugOutput *EngineCore::GetDebugOutput() const {
  return debug_output_.get();
}

void EngineCore::InitEngine() {
  auto gfx_mgr = lib_graphics::GraphicsFactory();
  auto phy_mgr = lib_physics::PhysicsFactory();
  auto inp_mgr = lib_input::InputFactory();
  auto sou_mgr = lib_sound::SoundFactory();
  auto gui_mgr = lib_gui::GuiFactory();

  window_ = gfx_mgr.CreateAppWindow();
  debug_output_ = std::make_unique<EngineDebugOutput>();

  auto mesh_up = gfx_mgr.CreateMeshSystem(this);
  auto material_up = gfx_mgr.CreateMaterialSystem(this);
  auto text_up = gui_mgr.CreateTextSystem();
  auto rect_up = gui_mgr.CreateRectSystem();
  auto sound_up = sou_mgr.CreateSoundSystem();
  auto input_up = inp_mgr.CreateInputSystem(this);
  auto camera_up = gfx_mgr.CreateCameraSystem();
  auto physic_up = phy_mgr.CreatePhysicsSystem();
  auto trans_up = gfx_mgr.CreateTransformSystem();
  auto light_up = gfx_mgr.CreateLightSystem();
  auto cull_up = gfx_mgr.CreateCullingSystem(this);
  auto ps_up = gfx_mgr.CreateParticleSystem(this);

  particle_system_ = ps_up.get();
  input_system_ = input_up.get();
  physics_system_ = physic_up.get();
  text_system_ = text_up.get();
  rect_system_ = rect_up.get();
  camera_system_ = camera_up.get();
  transform_system_ = trans_up.get();
  culling_system_ = cull_up.get();
  light_system_ = light_up.get();
  sound_system_ = sound_up.get();
  material_system_ = material_up.get();
  mesh_system_ = mesh_up.get();

  lib_core::AddSystemCommand mesh_command(std::move(mesh_up),
                                          lib_core::AddSystemCommand::Prerun);
  issue_command(mesh_command);
  lib_core::AddSystemCommand text_command(std::move(text_up),
                                          lib_core::AddSystemCommand::Prerun);
  issue_command(std::move(text_command));
  lib_core::AddSystemCommand material_command(
      std::move(material_up), lib_core::AddSystemCommand::Prerun);
  issue_command(std::move(material_command));
  lib_core::AddSystemCommand rect_command(std::move(rect_up),
                                          lib_core::AddSystemCommand::Prerun);
  issue_command(std::move(rect_command));
  lib_core::AddSystemCommand sound_command(std::move(sound_up),
                                           lib_core::AddSystemCommand::Prerun);
  issue_command(std::move(sound_command));
  lib_core::AddSystemCommand ps_command(std::move(ps_up),
                                        lib_core::AddSystemCommand::Prerun);
  issue_command(std::move(ps_command));

  lib_core::AddSystemCommand input_command(
      std::move(input_up), lib_core::AddSystemCommand::Prerender);
  issue_command(std::move(input_command));
  lib_core::AddSystemCommand camera_command(
      std::move(camera_up), lib_core::AddSystemCommand::Prerender);
  issue_command(std::move(camera_command));
  lib_core::AddSystemCommand culling_command(
      std::move(cull_up), lib_core::AddSystemCommand::Prerender);
  issue_command(std::move(culling_command));

  lib_core::AddSystemCommand physics_command(std::move(physic_up), 1000);
  issue_command(std::move(physics_command));
  lib_core::AddSystemCommand transform_command(std::move(trans_up), 1000);
  issue_command(std::move(transform_command));
  lib_core::AddSystemCommand light_command(std::move(light_up), 1000);
  issue_command(std::move(light_command));

  particle_system_id_ = ps_command.SystemId();
  input_system_id_ = input_command.SystemId();
  physics_system_id_ = physics_command.SystemId();
  text_system_id_ = text_command.SystemId();
  rect_system_id_ = rect_command.SystemId();
  mesh_system_id_ = mesh_command.SystemId();
  camera_system_id_ = camera_command.SystemId();
  transform_system_id_ = transform_command.SystemId();
  material_system_id_ = material_command.SystemId();
  culling_system_id_ = culling_command.SystemId();
  light_system_id_ = light_command.SystemId();
  sound_system_id_ = sound_command.SystemId();

  auto stock_textures =
      material_system_->LoadTexturePack("./content/stock_texpack");
  stock_texture = stock_textures[1].first;

  auto mesh_ids = mesh_system_->LoadModelPack("./content/stock_modelpack");
  stock_box_mesh = mesh_ids[0];
  stock_sphere_mesh = mesh_ids[1];
}
}  // namespace lib_core
