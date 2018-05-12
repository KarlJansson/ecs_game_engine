#include "gl_deferred_lighting.h"
#include <GL/glew.h>
#include "culling_system.h"
#include "gl_material_system.h"
#include "light_system.h"
#include "mesh_system.h"
#include "window.h"

namespace lib_graphics {
GlDeferredLighting::GlDeferredLighting(lib_core::EngineCore *engine,
                                       TextureDesc position_tex,
                                       TextureDesc normal_tex,
                                       TextureDesc albedo_tex,
                                       TextureDesc rme_tex,
                                       TextureDesc depth_tex)
    : engine_(engine) {
  ct::string vertex_header_ =
      "#version 330 core\n"
      "layout(location = 0) in vec3 position;\n"
      "layout(location = 1) in vec3 normal;\n"
      "layout(location = 2) in vec3 tangent;\n"
      "layout(location = 3) in vec2 texcoord;\n\n";

  ct::string pbr_lighting_attenuation_ =
      "    vec3 L = normalize(light_position[i] - frag_pos);\n"
      "    vec3 H = normalize(V + L);\n"
      "    float distance = length(light_position[i] - frag_pos);\n"
      "    float attenuation = 1.0 / (light_coeff[i].x + light_coeff[i].y * "
      "distance + light_coeff[i].z * (distance * distance));\n"
      "    attenuation *= clamp(1. - ((1. / light_radius[i]) * "
      "distance),0.,1.);\n"
      "    vec3 radiance = light_color[i] * clamp(attenuation,0.0,1.0);\n\n"

      "    float NDF = DistributionGGX(N, H, r);\n"
      "    float G = GeometrySmith(N, V, L, r);\n"
      "    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);\n\n"

      "    vec3 kS = F;\n"
      "    vec3 kD = vec3(1.0) - kS;\n"
      "    kD *= 1.0 - m;\n"

      "    vec3 nominator = NDF * G * F;\n"
      "    float denominator = max(dot(N, V), 0.0) * max(dot(N, "
      "L), 0.0) + 0.001;\n"
      "    vec3 brdf = nominator / denominator;\n\n"

      "    float NdotL = max(dot(N, L), 0.0);\n";

  ct::string pbr_lighting_spot_ =
      "    vec3 L = normalize(light_position[i] - frag_pos);\n"
      "    vec3 H = normalize(V + L);\n"

      "    float theta     = dot(L, normalize(-light_directions[i]));\n"
      "    float epsilon = light_cutoffs[i].x - light_cutoffs[i].y;\n"
      "    float intensity = clamp((theta - light_cutoffs[i].y) / epsilon, "
      "0.0,1.0);\n"

      "    float distance = length(light_position[i] - frag_pos);\n"
      "    float attenuation = 1.0 / (light_coeff[i].x + light_coeff[i].y * "
      "distance + light_coeff[i].z * (distance * distance));\n"
      "    attenuation *= 1.0 - ((1.0 / light_radius[i]) * distance);\n"
      "    vec3 radiance = light_color[i] * clamp(attenuation,0.0,1.0) "
      "*intensity;\n\n"

      "    float NDF = DistributionGGX(N, H, r);\n"
      "    float G = GeometrySmith(N, V, L, r);\n"
      "    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);\n\n"

      "    vec3 kS = F;\n"
      "    vec3 kD = vec3(1.0) - kS;\n"
      "    kD *= 1.0 - m;\n"

      "    vec3 nominator = NDF * G * F;\n"
      "    float denominator = max(dot(N, V), 0.0) * max(dot(N, "
      "L), 0.0) + 0.001;\n"
      "    vec3 brdf = nominator / denominator;\n\n"

      "    float NdotL = max(dot(N, L), 0.0);\n";

  ct::string pbr_lighting_dir_ =
      "    vec3 L = normalize(-light_directions[i]);\n"
      "    vec3 H = normalize(V + L);\n"
      "    vec3 radiance = light_color[i];\n\n"

      "    float NDF = DistributionGGX(N, H, r);\n"
      "    float G = GeometrySmith(N, V, L, r);\n"
      "    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);\n\n"

      "    vec3 kS = F;\n"
      "    vec3 kD = vec3(1.0) - kS;\n"
      "    kD *= 1.0 - m;\n"

      "    vec3 nominator = NDF * G * F;\n"
      "    float denominator = max(dot(N, V), 0.0) * max(dot(N, "
      "L), 0.0) + 0.001;\n"
      "    vec3 brdf = nominator / denominator;\n\n"

      "    float NdotL = max(dot(N, L), 0.0);\n";

  ct::string point_shadows_ =
      "float ShadowCalculation(vec3 fragPos, int i)\n"
      "{\n"
      "  vec3 fragToLight = fragPos - light_position[i];\n"
      "  float currentDepth = length(fragToLight);\n"
      "  float bias = 0.05;\n"
      "  float shadow = 0.0;\n"
      "  float samples = 4.0;\n"
      "  float offset = 0.1;\n"
      "  for (float x = -offset; x < offset; x += offset / (samples * 0.5))\n"
      "  {\n"
      "    for (float y = -offset; y < offset; y += offset / (samples * 0.5))\n"
      "    {\n"
      "	     for (float z = -offset; z < offset; z += offset / (samples "
      "* 0.5))\n"
      "	     {\n"
      "        float closestDepth = texture(depth_map[i], fragToLight + "
      "vec3(x, y, z)).r;\n"
      "		   closestDepth *= far_plane[i];\n"
      "		   if (currentDepth - bias > closestDepth)\n"
      "		     shadow += 1.0;\n"
      "	     }\n"
      "    }\n"
      "  }\n"
      "  shadow /= (samples * samples * samples);\n"

      "  return shadow;\n"
      "}\n\n";

  ct::string dir_shadows_ =
      "float ShadowCalculation(vec4 fragPos, float ndotl, int i)\n"
      "{\n"
      "  vec3 projCoords = fragPos.xyz / fragPos.w;\n"
      "  projCoords = projCoords * 0.5 + 0.5;\n"
      "  float currentDepth = projCoords.z;\n"

      "  float bias;\n"
      "  if(i == 0) bias = 0.001;\n"
      "  else if(i == 1) bias = 0.001;\n"
      "  else if(i == 2) bias = 0.01;\n"

      //"  float bias = 0.001;\n//max(0.01 * (1.0 - ndotl), 0.0001);\n"
      "  vec2 texelSize = 1.0/textureSize(depth_map[i], 0);\n"

      "  float shadow = 0.0;\n"
      "  for (int x = -1; x <= 1; ++x)\n"
      "  {\n"
      "    for (int y = -1; y <= 1; ++y)\n"
      "    {\n"
      "	    shadow += texture(depth_map[i], vec3(projCoords.xy + vec2(x, "
      "y) * texelSize, currentDepth - bias));\n"
      "    }\n"
      "  }\n"
      "  shadow *= 0.25;\n"
      "  return shadow;\n"
      "}";

  ct::string pbr_helper_funcs_ =
      "const float PI = 3.14159265359;\n\n"
      "vec3 fresnelSchlick(float cosTheta, vec3 F0)\n"
      "{\n"
      "  return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);\n"
      "}\n\n"

      "float DistributionGGX(vec3 N, vec3 H, float roughness)\n"
      "{\n"
      "  float a = roughness*roughness;\n"
      "  float a2 = a*a;\n"
      "  float NdotH = max(dot(N, H), 0.0);\n"
      "  float NdotH2 = NdotH*NdotH;\n\n"

      "  float nom = a2;\n"
      "  float denom = (NdotH2 * (a2 - 1.0) + 1.0);\n"
      "  denom = PI * denom * denom;\n\n"

      "  return nom / denom;\n"
      "}\n\n"

      "float "
      "GeometrySchlickGGX(float NdotV, float roughness)\n"
      "{\n"
      "  float r = (roughness + 1.0);\n"
      "  float k = (r*r) / 8.0;\n\n"

      "  float nom = NdotV;\n"
      "  float denom = NdotV * (1.0 - k) + k;\n\n"

      "  return nom / denom;\n"
      "}\n\n"

      "float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)\n"
      "{\n"
      "  float NdotV = max(dot(N, V), 0.0);\n"
      "  float NdotL = max(dot(N, L), 0.0);\n"
      "  float ggx2 = GeometrySchlickGGX(NdotV, roughness);\n"
      "  float ggx1 = GeometrySchlickGGX(NdotL, roughness);\n\n"

      "  return ggx1 * ggx2;\n"
      "}\n\n";

  ct::string light_variables_point_ =
      "uniform vec3 light_position[50];\n"
      "uniform vec3 light_coeff[50];\n"
      "uniform float light_radius[50];\n"
      "uniform vec3 light_color[50];\n\n";

  ct::string light_shadows_variables_point_ =
      "uniform samplerCube depth_map[5];\n"
      "uniform float far_plane[5];\n";

  ct::string light_variables_dir_ =
      "uniform vec3 light_directions[50];\n"
      "uniform vec3 light_color[50];\n\n";

  ct::string light_shadows_variables_dir_ =
      "uniform mat4 world[3];\n"
      "uniform sampler2DShadow depth_map[3];\n"
      "uniform float cascade_depth[3];\n";

  ct::string light_variables_spot_ =
      light_variables_dir_ + "uniform vec2 light_cutoffs[50];\n\n";

  ct::string deferred_fragment_header =
      "#version 330 core\n"
      "out vec4 FragColor;\n"
      "flat in int inst_id;\n"

      "uniform sampler2D g_position;\n"
      "uniform sampler2D g_normal;\n"
      "uniform sampler2D g_albedo;\n"
      "uniform sampler2D g_rma;\n"
      "uniform sampler2D g_depth;\n";

  ct::string deferred_fragment_start =
      "  vec3 frag_pos = texture(g_position, frag_coord).rgb;\n"
      "  vec3 N = normalize(texture(g_normal, frag_coord).rgb * 2 - 1);\n"

      "  vec3 V = normalize(cam_pos - frag_pos);\n\n"

      "  vec3 rma = texture(g_rma, frag_coord).rgb;\n"
      "  vec3 albedo = pow(texture(g_albedo, frag_coord).rgb, vec3(2.2));\n"

      "  float r = clamp(rma.r, 0.05, 1.0);\n"
      "  float m = rma.g;\n"
      "  float e = rma.b;\n\n"
      "  int i = inst_id;\n"

      "  vec3 F0 = vec3(0.04);\n"
      "  F0 = mix(F0, albedo, m);\n"
      "  vec3 Lo = vec3(0.0);\n";

  ct::string vert_shader =
      vertex_header_ +

      "uniform mat4 world[50];\n"
      "uniform mat4 view_proj;\n"

      "flat out int inst_id;\n"

      "void main()\n"
      "{\n"
      "  gl_Position = view_proj * (world[gl_InstanceID] * "
      "vec4(position, 1.0));\n"
      "  inst_id = gl_InstanceID;\n"
      "}";

  ct::string frag_shader =
      deferred_fragment_header +

      "uniform vec3 cam_pos;\n"
      "uniform vec2 screen_dim;\n"

      + light_shadows_variables_point_ + light_variables_point_ +
      point_shadows_ + pbr_helper_funcs_ +

      "void main()\n"
      "{\n"
      "  vec2 frag_coord = gl_FragCoord.xy / screen_dim;\n"

      + deferred_fragment_start + pbr_lighting_attenuation_ +

      "  float shadow = 1.0 - ShadowCalculation(frag_pos, i);\n"
      "  Lo = shadow * (kD * albedo / PI + brdf) * radiance * NdotL;\n"
      "  FragColor = vec4(Lo, 1.0);\n"
      "}";

  Material material;

  auto shader_command = AddShaderCommand(vert_shader);
  issue_command(shader_command);
  shader_ids_.push_back(shader_command.ShaderId());

  material.shader = shader_command.ShaderId();

  auto material_command = AddMaterialCommand(material);
  issue_command(material_command);
  stencil_pass_ = material_command.MaterialId();

  shader_command = AddShaderCommand(vert_shader, frag_shader);
  issue_command(shader_command);
  shader_ids_.push_back(shader_command.ShaderId());
  material.shader = shader_command.ShaderId();
  material.textures.push_back(position_tex);
  material.textures.push_back(normal_tex);
  material.textures.push_back(albedo_tex);
  material.textures.push_back(rme_tex);
  material.textures.push_back(depth_tex);

  material_command = AddMaterialCommand(material);
  issue_command(material_command);
  deferred_lighting_point_shadow_volume_ = material_command.MaterialId();

  frag_shader = deferred_fragment_header +

                "uniform vec3 cam_pos;\n"
                "uniform vec2 screen_dim;\n"

                + light_variables_point_ + pbr_helper_funcs_ +

                "void main()\n"
                "{\n"
                "  vec2 frag_coord = gl_FragCoord.xy / screen_dim;\n"

                + deferred_fragment_start + pbr_lighting_attenuation_ +

                "  Lo = (kD * albedo / PI + brdf) * radiance * NdotL;\n"
                "  FragColor = vec4(Lo, 1.0);\n"
                "}";

  shader_command = AddShaderCommand(vert_shader, frag_shader);
  issue_command(shader_command);
  shader_ids_.push_back(shader_command.ShaderId());
  material.shader = shader_command.ShaderId();

  material_command = AddMaterialCommand(material);
  issue_command(material_command);
  deferred_lighting_volume_ = material_command.MaterialId();

  vert_shader =
      "#version 330 core\n"
      "layout(location = 0) in vec2 position;\n"
      "layout(location = 1) in vec2 texCoords;\n"

      "out vec2 TexCoords;\n"
      "flat out int inst_id;\n"

      "void main()\n"
      "{\n"
      "  gl_Position = vec4(position.x, position.y, 0.0f, 1.0f);\n"
      "  TexCoords = texCoords;\n"
      "  inst_id = gl_InstanceID;\n"
      "}";

  frag_shader =
      deferred_fragment_header +
      "in vec2 TexCoords;\n"

      "uniform vec3 cam_pos;\n"

      + light_shadows_variables_point_ + light_variables_point_ +
      point_shadows_ + pbr_helper_funcs_ +

      "void main()\n"
      "{\n"
      "  vec2 frag_coord = TexCoords;\n"

      + deferred_fragment_start + pbr_lighting_attenuation_ +

      "  float shadow = 1.0 - ShadowCalculation(frag_pos, i);\n"
      "  Lo = shadow * (kD * albedo / PI + brdf) * radiance * NdotL;\n"
      "  FragColor = vec4(Lo, 1.0);\n"
      "}";

  shader_command = AddShaderCommand(vert_shader, frag_shader);
  issue_command(shader_command);
  shader_ids_.push_back(shader_command.ShaderId());

  material.shader = shader_command.ShaderId();

  material_command = AddMaterialCommand(material);
  issue_command(material_command);
  deferred_lighting_point_shadow_quad_ = material_command.MaterialId();

  frag_shader = deferred_fragment_header +
                "in vec2 TexCoords;\n"

                "uniform vec3 cam_pos;\n"

                + light_variables_point_ + pbr_helper_funcs_ +

                "void main()\n"
                "{\n"
                "  vec2 frag_coord = TexCoords;\n"

                + deferred_fragment_start + pbr_lighting_attenuation_ +

                "  Lo = (kD * albedo / PI + brdf) * radiance * NdotL;\n"
                "  FragColor = vec4(Lo, 1.0);\n"
                "}";

  shader_command = AddShaderCommand(vert_shader, frag_shader);
  issue_command(shader_command);
  shader_ids_.push_back(shader_command.ShaderId());
  material.shader = shader_command.ShaderId();

  material_command = AddMaterialCommand(material);
  issue_command(material_command);
  deferred_lighting_quad_ = material_command.MaterialId();

  frag_shader =
      deferred_fragment_header +
      "in vec2 TexCoords;\n"

      "uniform vec3 cam_pos;\n"

      + light_variables_dir_ + light_shadows_variables_dir_ + dir_shadows_ +
      pbr_helper_funcs_ +

      "void main()\n"
      "{\n"
      "  vec2 frag_coord = TexCoords;\n"

      + deferred_fragment_start + pbr_lighting_dir_ +

      "  float depth = texture(g_depth, frag_coord).r;\n"

      "  for(int l = 0; l<3; ++l)\n"
      "  {\n"
      "    if(depth < cascade_depth[l] || l == 2)\n"
      "    {\n"
      "      vec4 light_pos = world[l] * vec4(frag_pos, 1.0);\n"
      "      float shadow = ShadowCalculation(light_pos, NdotL, l);\n"
      "      Lo = shadow * (kD * albedo / PI + brdf) * radiance * NdotL;\n"
      "      break;"
      "    }\n"
      "  }\n"

      "  FragColor = vec4(Lo, 1.0);\n"
      "}";

  shader_command = AddShaderCommand(vert_shader, frag_shader);
  issue_command(shader_command);
  shader_ids_.push_back(shader_command.ShaderId());
  material.shader = shader_command.ShaderId();

  material_command = AddMaterialCommand(material);
  issue_command(material_command);
  deferred_lighting_dir_shadow_quad_ = material_command.MaterialId();

  frag_shader = deferred_fragment_header +
                "in vec2 TexCoords;\n"

                "uniform vec3 cam_pos;\n"

                + light_variables_dir_ + pbr_helper_funcs_ +

                "void main()\n"
                "{\n"
                "  vec2 frag_coord = TexCoords;\n"

                + deferred_fragment_start + pbr_lighting_dir_ +

                "  Lo = (kD * albedo / PI + brdf) * radiance * NdotL;\n"
                "  FragColor = vec4(Lo, 1.0);\n"
                "}";

  shader_command = AddShaderCommand(vert_shader, frag_shader);
  issue_command(shader_command);
  shader_ids_.push_back(shader_command.ShaderId());
  material.shader = shader_command.ShaderId();

  material_command = AddMaterialCommand(material);
  issue_command(material_command);
  deferred_lighting_dir_quad_ = material_command.MaterialId();

  shadow_mapper_ = std::make_unique<GlShadowMapping>(engine_);
}

GlDeferredLighting::~GlDeferredLighting() {
  for (auto &id : shader_ids_) issue_command(RemoveShaderCommand(id));

  issue_command(RemoveMaterialCommand(deferred_lighting_point_shadow_volume_));
  issue_command(RemoveMaterialCommand(deferred_lighting_point_shadow_quad_));
  issue_command(RemoveMaterialCommand(deferred_lighting_dir_shadow_quad_));
  issue_command(RemoveMaterialCommand(deferred_lighting_dir_quad_));
  issue_command(RemoveMaterialCommand(deferred_lighting_volume_));
  issue_command(RemoveMaterialCommand(deferred_lighting_quad_));
  issue_command(RemoveMaterialCommand(stencil_pass_));
}

void GlDeferredLighting::DrawLights(Camera &cam, lib_core::Entity cam_entity,
                                    float cam_pos[3]) {
  auto lighting_timer = cu::TimerStart();
  auto cull_system = engine_->GetCulling();

  auto &light_matrices = cull_system->GetLightMatrices(cam_entity);
  auto lights = cull_system->GetLightPacks(cam_entity);

  light_mats.clear();
  light_packs.clear();
  int light_matrix_id = 0;
  for (auto light_ent : *lights) {
    auto light = g_ent_mgr.GetOldCbeR<Light>(light_ent);
    if (!light) continue;

    size_t material;
    if (light->type == Light::kPoint) {
      auto cam_d = lib_core::Vector3(cam_pos[0], cam_pos[1], cam_pos[2]);
      cam_d -= light->data_pos;
      auto cam_dist = cam_d.Length();
      bool inside = light->max_radius + 1.0f > cam_dist;

      material =
          light->cast_shadows
              ? (inside ? deferred_lighting_point_shadow_quad_
                        : deferred_lighting_point_shadow_volume_)
              : (inside ? deferred_lighting_quad_ : deferred_lighting_volume_);
      light_mats[material].push_back(light_matrices[light_matrix_id++]);
    } else if (light->type == Light::kDir) {
      material = light->cast_shadows ? deferred_lighting_dir_shadow_quad_
                                     : deferred_lighting_dir_quad_;
      light_mats[material].push_back(light_matrices[light_matrix_id++]);
      light_mats[material].push_back(light_matrices[light_matrix_id++]);
      light_mats[material].push_back(light_matrices[light_matrix_id++]);
    }
    light_packs[material].push_back({light_ent, *light});
  }

  InstanceDraw(cam, cam_entity, cam_pos);
  engine_->GetDebugOutput()->UpdateBottomRightLine(
      2, std::to_string(cu::TimerStop<std::milli>(lighting_timer)) +
             " :Lighting time");
}

void GlDeferredLighting::SetScreenQuad(unsigned quad) { screen_quad_ = quad; }

void GlDeferredLighting::StencilDraw(Camera &cam, lib_core::Entity cam_entity,
                                     float cam_pos[3]) {
  auto mat_system = static_cast<GlMaterialSystem *>(engine_->GetMaterial());
  auto mesh_system = engine_->GetMesh();
  auto window = engine_->GetWindow();

  glStencilOpSeparate(GL_BACK, GL_KEEP, GL_INCR_WRAP, GL_KEEP);
  glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_DECR_WRAP, GL_KEEP);
  for (auto &l_pack : light_packs) {
    int inst_count = 0;
    int max_instance =
        1;  // l_pack.second[0].second.type == Light::kDir ? 5 : 50;
    ct::dyn_array<std::pair<lib_core::Entity, Light>> light_pack;
    for (int iii = 0; iii < l_pack.second.size(); iii += max_instance) {
      light_pack.clear();
      for (int ii = iii; ii < l_pack.second.size(); ++ii) {
        light_pack.push_back(l_pack.second[ii]);
        if (light_pack.size() == max_instance) break;
      }

      // Render shadows
      if (l_pack.first == deferred_lighting_dir_shadow_quad_ ||
          l_pack.first == deferred_lighting_point_shadow_volume_ ||
          l_pack.first == deferred_lighting_point_shadow_quad_) {
        glDepthMask(GL_TRUE);
        glCullFace(GL_BACK);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glDisable(GL_STENCIL_TEST);
        shadow_mapper_->DrawShadowMap(light_pack[0]);
      }

      glDepthMask(GL_FALSE);
      auto &mats = light_mats[l_pack.first];
      if (l_pack.first != deferred_lighting_dir_shadow_quad_ &&
          l_pack.first != deferred_lighting_dir_quad_) {
        glEnable(GL_STENCIL_TEST);
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glStencilFunc(GL_ALWAYS, 0, 0xFF);

        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glClear(GL_STENCIL_BUFFER_BIT);

        mat_system->ApplyMaterial(stencil_pass_, nullptr, true);

        auto shader_program = mat_system->GetCurrentShader();
        if (stencil_vp_loc_ - 1)
          stencil_vp_loc_ = glGetUniformLocation(shader_program, "view_proj");
        glUniformMatrix4fv(stencil_vp_loc_, 1, GL_FALSE, cam.view_proj_.data);

        if (stencil_world_loc_ == -1)
          stencil_world_loc_ = glGetUniformLocation(shader_program, "world[0]");
        glUniformMatrix4fv(stencil_world_loc_, int(light_pack.size()), GL_FALSE,
                           (float *)&mats[inst_count]);

        mesh_system->DrawMesh(lib_core::EngineCore::stock_sphere_mesh,
                              int(light_pack.size()), true);
      }

      glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
      glDisable(GL_DEPTH_TEST);
      glEnable(GL_BLEND);
      glBlendFunc(GL_ONE, GL_ONE);
      glEnable(GL_CULL_FACE);

      if (l_pack.first != deferred_lighting_dir_shadow_quad_ &&
          l_pack.first != deferred_lighting_dir_quad_) {
        if (l_pack.first == deferred_lighting_quad_ ||
            l_pack.first == deferred_lighting_point_shadow_quad_) {
          glCullFace(GL_BACK);
          glStencilFunc(GL_LESS, 0, 0xFF);
        } else {
          glCullFace(GL_FRONT);
          glStencilFunc(GL_NOTEQUAL, 0, 0xFF);
        }
      } else {
        glCullFace(GL_BACK);
        glDisable(GL_STENCIL_TEST);
      }

      mat_system->ApplyMaterial(l_pack.first, &light_pack, true);
      auto shader_program = mat_system->GetCurrentShader();

      auto loc_it = shader_locations_.find(shader_program);
      if (loc_it == shader_locations_.end()) {
        auto &data = shader_locations_[shader_program];
        data[0] = glGetUniformLocation(shader_program, "cam_pos");
        data[1] = glGetUniformLocation(shader_program, "screen_dim");
        data[2] = glGetUniformLocation(shader_program, "view_proj");
        data[3] = glGetUniformLocation(shader_program, "world[0]");
        loc_it = shader_locations_.find(shader_program);
      }

      float scr_dim[] = {float(window->GetRenderDim().first),
                         float(window->GetRenderDim().second)};

      glUniform3fv(loc_it->second[0], 1, &cam_pos[0]);
      glUniform2fv(loc_it->second[1], 1, scr_dim);
      glUniformMatrix4fv(loc_it->second[2], 1, GL_FALSE, cam.view_proj_.data);
      if (l_pack.first == deferred_lighting_dir_shadow_quad_)
        glUniformMatrix4fv(loc_it->second[3], 3, GL_FALSE,
                           mats[inst_count].data);
      else
        glUniformMatrix4fv(loc_it->second[3], int(light_pack.size()), GL_FALSE,
                           (float *)&mats[inst_count]);

      inst_count += int(light_pack.size());

      if (l_pack.first == deferred_lighting_point_shadow_quad_ ||
          l_pack.first == deferred_lighting_quad_ ||
          l_pack.first == deferred_lighting_dir_shadow_quad_ ||
          l_pack.first == deferred_lighting_dir_quad_) {
        glBindVertexArray(screen_quad_);
        glDrawArraysInstanced(GL_TRIANGLES, 0, 3, int(light_pack.size()));
      } else
        mesh_system->DrawMesh(lib_core::EngineCore::stock_sphere_mesh,
                              int(light_pack.size()), true);
    }
  }

  glDisable(GL_STENCIL_TEST);
  glEnable(GL_DEPTH_TEST);
  glCullFace(GL_BACK);
  glDisable(GL_BLEND);
  glDepthMask(GL_TRUE);
}

void GlDeferredLighting::InstanceDraw(Camera &cam, lib_core::Entity cam_entity,
                                      float cam_pos[3]) {
  auto mat_system = engine_->GetMaterial();
  auto mesh_system = engine_->GetMesh();
  auto window = engine_->GetWindow();

  for (auto &l_pack : light_packs) {
    int inst_count = 0;
    int max_instance = l_pack.second[0].second.type == Light::kDir ? 5 : 50;
    if (l_pack.first == deferred_lighting_dir_shadow_quad_ ||
        l_pack.first == deferred_lighting_point_shadow_volume_ ||
        l_pack.first == deferred_lighting_point_shadow_quad_)
      max_instance = 1;

    ct::dyn_array<std::pair<lib_core::Entity, Light>> light_pack;
    for (int iii = 0; iii < l_pack.second.size(); iii += max_instance) {
      light_pack.clear();
      for (int ii = iii; ii < l_pack.second.size(); ++ii) {
        light_pack.push_back(l_pack.second[ii]);
        if (light_pack.size() == max_instance) break;
      }

      // Render shadows
      if (l_pack.first == deferred_lighting_dir_shadow_quad_ ||
          l_pack.first == deferred_lighting_point_shadow_volume_ ||
          l_pack.first == deferred_lighting_point_shadow_quad_) {
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        shadow_mapper_->DrawShadowMap(light_pack[0]);
      }

      glDepthMask(GL_FALSE);
      glEnable(GL_BLEND);
      glBlendFunc(GL_ONE, GL_ONE);

      auto &mats = light_mats[l_pack.first];
      mat_system->ApplyMaterial(l_pack.first, &light_pack, true);
      auto shader_program = mat_system->GetCurrentShader();

      auto loc_it = shader_locations_.find(shader_program);
      if (loc_it == shader_locations_.end()) {
        auto &data = shader_locations_[shader_program];
        data[0] = glGetUniformLocation(shader_program, "cam_pos");
        data[1] = glGetUniformLocation(shader_program, "screen_dim");
        data[2] = glGetUniformLocation(shader_program, "view_proj");
        data[3] = glGetUniformLocation(shader_program, "world[0]");
        loc_it = shader_locations_.find(shader_program);
      }

      float scr_dim[] = {float(window->GetRenderDim().first),
                         float(window->GetRenderDim().second)};

      glUniform3fv(loc_it->second[0], 1, &cam_pos[0]);
      glUniform2fv(loc_it->second[1], 1, scr_dim);
      glUniformMatrix4fv(loc_it->second[2], 1, GL_FALSE, cam.view_proj_.data);
      if (l_pack.first == deferred_lighting_dir_shadow_quad_)
        glUniformMatrix4fv(loc_it->second[3], 3, GL_FALSE,
                           mats[inst_count].data);
      else
        glUniformMatrix4fv(loc_it->second[3], int(light_pack.size()), GL_FALSE,
                           (float *)&mats[inst_count]);

      inst_count += int(light_pack.size());

      if (l_pack.first == deferred_lighting_point_shadow_quad_ ||
          l_pack.first == deferred_lighting_quad_ ||
          l_pack.first == deferred_lighting_dir_shadow_quad_ ||
          l_pack.first == deferred_lighting_dir_quad_) {
        glBindVertexArray(screen_quad_);
        glDrawArraysInstanced(GL_TRIANGLES, 0, 3, int(light_pack.size()));
      } else
        mesh_system->DrawMesh(lib_core::EngineCore::stock_sphere_mesh,
                              int(light_pack.size()), true);
    }
  }

  glDisable(GL_BLEND);
  glDepthMask(GL_TRUE);
}
}  // namespace lib_graphics
