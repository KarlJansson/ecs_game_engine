#pragma once
#include "system_manager.h"
#include "vector_def.h"
#include "vertex.h"

namespace lib_gui {
class TextSystem;
}

namespace lib_graphics {
class CreateTextureFrameBufferCommand : public lib_core::Command {
 public:
  CreateTextureFrameBufferCommand() = default;
  CreateTextureFrameBufferCommand(size_t height, size_t width)
      : width(width), height(height) {
    base_id = g_sys_mgr.GenerateResourceIds(2);
  }

  size_t FrameBufferId() { return base_id; }
  size_t TextureId() { return base_id + 1; }

  size_t width;
  size_t height;
};

class FreeTextureFrameBufferCommand : public lib_core::Command {
 public:
  FreeTextureFrameBufferCommand() = default;
  FreeTextureFrameBufferCommand(size_t fb_id) : frame_buffer_id(fb_id) {}
  size_t frame_buffer_id;
};

class RenderToTextureCommand : public lib_core::Command {
 public:
  RenderToTextureCommand() = default;
  RenderToTextureCommand(
      size_t fb_id,
      std::function<void(class Renderer* renderer, lib_gui::TextSystem* text)>
          func)
      : frame_buffer(fb_id), render_function(func) {}

  size_t frame_buffer;
  std::function<void(class Renderer* renderer, lib_gui::TextSystem* text)>
      render_function;
};

struct TextureDesc {
  size_t id;
  ct::string name;
};

struct Material {
  size_t shader = 0;
  ct::dyn_array<TextureDesc> textures;
  ct::dyn_array<std::pair<ct::string, float>> float_var;
  ct::dyn_array<std::pair<ct::string, lib_core::Vector3>> float_vec3;
  ct::dyn_array<size_t> frame_buffers;
};

struct MeshInit {
  ct::dyn_array<uint32_t> indices;
  ct::dyn_array<Vertex> vertices;
  lib_core::Vector3 center, extent;
};

class AddMaterialCommand : public lib_core::Command {
 public:
  AddMaterialCommand() = default;
  AddMaterialCommand(Material material) : material(std::move(material)) {
    base_id = g_sys_mgr.GenerateResourceIds(1);
  }

  size_t MaterialId() { return base_id; }

  Material material;
};

class UpdateMaterialCommand : public lib_core::Command {
 public:
  UpdateMaterialCommand() = default;
  UpdateMaterialCommand(size_t material_id, Material material)
      : material_id(material_id), material(std::move(material)) {}

  size_t material_id;
  Material material;
};

class RemoveMaterialCommand : public lib_core::Command {
 public:
  RemoveMaterialCommand() = default;
  RemoveMaterialCommand(size_t material_id) : material_id(material_id) {}

  size_t material_id;
};

class AddShaderCommand : public lib_core::Command {
 public:
  AddShaderCommand() = default;
  AddShaderCommand(ct::string vert_shader, ct::string frag_shader = "",
                   ct::string geom_shader = "")
      : vert_shader(vert_shader),
        frag_shader(frag_shader),
        geom_shader(geom_shader) {
    base_id = g_sys_mgr.GenerateResourceIds(1);
  }

  size_t ShaderId() { return base_id; }

  ct::string vert_shader, frag_shader, geom_shader;
};

class RemoveShaderCommand : public lib_core::Command {
 public:
  RemoveShaderCommand() = default;
  RemoveShaderCommand(size_t shader_id) : shader_id(shader_id) {}

  size_t shader_id;
};

class AddMeshCommand : public lib_core::Command {
 public:
  AddMeshCommand() = default;
  AddMeshCommand(MeshInit mesh_init) : mesh_init(mesh_init) {
    base_id = g_sys_mgr.GenerateResourceIds(1);
  }

  size_t MeshId() { return base_id; }

  MeshInit mesh_init;
};

class AddModelMeshCommand : public lib_core::Command {
 public:
  AddModelMeshCommand() = default;
  AddModelMeshCommand(size_t mesh_id, MeshInit mesh_init)
      : mesh_id(mesh_id), mesh_init(mesh_init) {}

  size_t MeshId() { return mesh_id; }

  size_t mesh_id;
  MeshInit mesh_init;
};

class RemoveMeshCommand : public lib_core::Command {
 public:
  RemoveMeshCommand() = default;
  RemoveMeshCommand(size_t mesh_id) : mesh_id(mesh_id) {}

  size_t mesh_id;
};

class CreateDeferedFrameBufferCommand : public lib_core::Command {
 public:
  CreateDeferedFrameBufferCommand() = default;
  CreateDeferedFrameBufferCommand(size_t width, size_t height)
      : width(width), height(height) {
    base_id = g_sys_mgr.GenerateResourceIds(6);
  }

  size_t FrameBufferId() { return base_id; }
  size_t PositionTextureId() { return base_id + 1; }
  size_t NormalTextureId() { return base_id + 2; }
  size_t AlbedoTextureId() { return base_id + 3; }
  size_t RmeTextureId() { return base_id + 4; }
  size_t DepthTextureId() { return base_id + 5; }

  size_t width;
  size_t height;
};

class CreateHdrFrameBufferCommand : public lib_core::Command {
 public:
  CreateHdrFrameBufferCommand() = default;
  CreateHdrFrameBufferCommand(size_t width, size_t height)
      : width(width), height(height) {
    base_id = g_sys_mgr.GenerateResourceIds(3);
  }

  size_t FrameBufferId() { return base_id; }
  size_t HdrTextureId() { return base_id + 1; }
  size_t DepthTextureId() { return base_id + 2; }

  size_t width;
  size_t height;
};

class CreateBlurFrameBufferCommand : public lib_core::Command {
 public:
  CreateBlurFrameBufferCommand() = default;
  CreateBlurFrameBufferCommand(size_t width, size_t height, int texture_type,
                               bool shadow)
      : width(width),
        height(height),
        texture_type(texture_type),
        shadow(shadow) {
    base_id = g_sys_mgr.GenerateResourceIds(4);
  }

  size_t FrameBuffer1Id() { return base_id; }
  size_t FrameBuffer2Id() { return base_id + 1; }
  size_t Texture1Id() { return base_id + 2; }
  size_t Texture2Id() { return base_id + 3; }

  size_t width;
  size_t height;
  int texture_type;
  bool shadow;
};
}  // namespace lib_graphics
