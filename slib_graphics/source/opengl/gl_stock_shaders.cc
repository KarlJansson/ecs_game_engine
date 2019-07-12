#include "gl_stock_shaders.h"
#include "material_system.h"
#include "renderer.h"

namespace lib_graphics {
GlStockShaders::GlStockShaders() = default;

void GlStockShaders::ForwardShaders() {
  shader_source_[MaterialSystem::kText] = {
      cu::ReadFile("./content/shaders/opengl/text_vs.glsl"),
      cu::ReadFile("./content/shaders/opengl/text_fs.glsl"), ""};

  shader_source_[MaterialSystem::kPbrUntextured] = {
      cu::ReadFile("./content/shaders/opengl/forward_pbr_untextured_vs.glsl"),
      cu::ReadFile("./content/shaders/opengl/forward_pbr_untextured_fs.glsl"),
      ""};

  shader_source_[MaterialSystem::kPbrTextured] = {
      cu::ReadFile("./content/shaders/opengl/forward_pbr_textured_vs.glsl"),
      cu::ReadFile("./content/shaders/opengl/forward_pbr_textured_fs.glsl"),
      ""};
}

void GlStockShaders::DeferredShaders() {
  shader_source_[MaterialSystem::kText] = {
      cu::ReadFile("./content/shaders/opengl/text_vs.glsl"),
      cu::ReadFile("./content/shaders/opengl/text_fs.glsl"), ""};

  shader_source_[MaterialSystem::kPbrTextured] = {
      cu::ReadFile("./content/shaders/opengl/deferred_pbr_textured_vs.glsl"),
      cu::ReadFile("./content/shaders/opengl/deferred_pbr_textured_fs.glsl"),
      ""};

  shader_source_[MaterialSystem::kPbrUntextured] = {
      cu::ReadFile("./content/shaders/opengl/deferred_pbr_untextured_vs.glsl"),
      cu::ReadFile("./content/shaders/opengl/deferred_pbr_untextured_fs.glsl"),
      ""};

  shader_source_[MaterialSystem::kDeferredLightingAmbient] = {
      cu::ReadFile("./content/shaders/opengl/deferred_ambient_tonemap_vs.glsl"),
      cu::ReadFile("./content/shaders/opengl/deferred_ambient_fs.glsl"), ""};

  shader_source_[MaterialSystem::kTonemapGamma] = {
      cu::ReadFile("./content/shaders/opengl/deferred_ambient_tonemap_vs.glsl"),
      cu::ReadFile("./content/shaders/opengl/deferred_tonemap_fs.glsl"), ""};
}

void GlStockShaders::CompileShaders(MaterialSystem *mat_mgr) {
  ForwardShaders();
  for (auto &p : shader_source_) {
    auto it = forward_shader_ids_.find(p.first);
    if (it == forward_shader_ids_.end()) {
      auto shader_command = AddShaderCommand(
          std::get<0>(p.second), std::get<1>(p.second), std::get<2>(p.second));
      forward_shader_ids_[p.first] = shader_command.ShaderId();
      issue_command(shader_command);
    } else {
      auto shader_command = AddShaderCommand();
      shader_command.vert_shader = std::get<0>(p.second);
      shader_command.frag_shader = std::get<1>(p.second);
      shader_command.geom_shader = std::get<2>(p.second);
      shader_command.base_id = it->second;
      issue_command(shader_command);
    }
  }
  shader_source_.clear();

  DeferredShaders();
  for (auto &p : shader_source_) {
    auto it = deferred_shader_ids_.find(p.first);
    if (it == deferred_shader_ids_.end()) {
      auto shader_command = AddShaderCommand(
          std::get<0>(p.second), std::get<1>(p.second), std::get<2>(p.second));
      deferred_shader_ids_[p.first] = shader_command.ShaderId();
      auto f_it = forward_shader_ids_.find(p.first);
      if (f_it != forward_shader_ids_.end())
        def_to_for_map_[shader_command.ShaderId()] = f_it->second;
      issue_command(shader_command);
    } else {
      auto shader_command = AddShaderCommand();
      shader_command.vert_shader = std::get<0>(p.second);
      shader_command.frag_shader = std::get<1>(p.second);
      shader_command.geom_shader = std::get<2>(p.second);
      shader_command.base_id = it->second;
      issue_command(shader_command);
    }
  }
  shader_source_.clear();
}

size_t GlStockShaders::GetShaderId(MaterialSystem::ShaderType type) {
  auto it = deferred_shader_ids_.find(type);
  return it != deferred_shader_ids_.end() ? it->second : 0;
}

size_t GlStockShaders::DefToForId(size_t deferred_id) {
  auto it = def_to_for_map_.find(deferred_id);
  if (it == def_to_for_map_.end()) return 0;
  return it->second;
}
}  // namespace lib_graphics
