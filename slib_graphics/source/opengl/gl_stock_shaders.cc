#include "gl_stock_shaders.h"
#include "material_system.h"
#include "renderer.h"

namespace lib_graphics {
GlStockShaders::GlStockShaders() {}

void GlStockShaders::CommonShaderSnippets() {
  vertex_header_ =
      "#version 330 core\n"
      "layout(location = 0) in vec3 position;\n"
      "layout(location = 1) in vec3 normal;\n"
      "layout(location = 2) in vec3 tangent;\n"
      "layout(location = 3) in vec2 texcoord;\n\n";

  matrix_input_ =
      "uniform mat4 world[100];\n"
      "uniform mat4 world_inv_trans[100];\n"
      "uniform mat4 view_proj;";

  pbr_lighting_attenuation_ =
      "    vec3 L = normalize(light_position[i] - frag_pos);\n"
      "    vec3 H = normalize(V + L);\n"
      "    float distance = length(light_position[i] - frag_pos);\n"
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

  pbr_lighting_dir_ =
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

  point_shadows_ =
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

  pbr_helper_funcs_ =
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

      "float GeometrySchlickGGX(float NdotV, float roughness)\n"
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

  pbr_util_funcs_ =
      "uniform int nr_lights;\n"
      "uniform vec3 light_position[4];\n"
      "uniform vec3 light_color[4];\n\n"
      "uniform samplerCube depth_map[4];\n"
      "uniform float far_plane[4];\n"
      "uniform int shadow_caster[4];\n"

      + point_shadows_ + pbr_helper_funcs_ +

      "vec3 PbrLighting(vec3 frag_pos, vec3 V, vec3 N, vec3 F0, vec3 a, "
      "float m, float r){"
      "  vec3 Lo = vec3(0.0);\n"
      "  for (int i = 0; i < nr_lights; ++i)\n"
      "  {\n" +
      pbr_lighting_attenuation_;

  pbr_util_funcs_shadows_end_ =
      "    float shadow = 1.0;\n"
      "    if(shadow_caster[i] == 1)\n"
      "      shadow = 1.0 - ShadowCalculation(frag_pos, i);\n"
      "    Lo += shadow * (kD * a / PI + brdf) * radiance * NdotL;\n"
      "  }\n"
      "  return Lo;\n"
      "}";

  pbr_util_funcs_end_ =
      "    Lo += (kD * a / PI + brdf) * radiance * NdotL;\n"
      "  }\n"
      "  return Lo;\n"
      "}";

  ct::string vert_shader =
      "#version 330 core\n"
      "layout(location = 0) in vec4 vertex;\n"
      "out vec2 TexCoords;\n\n"

      "uniform mat4 projection;\n\n"

      "void main(){\n"
      "  gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);\n"
      "  TexCoords = vertex.zw;\n"
      "}";

  ct::string frag_shader =
      "#version 330 core\n"
      "in vec2 TexCoords;\n"
      "out vec4 color;\n\n"

      "uniform sampler2D text;\n"
      "uniform vec3 textColor;\n\n"

      "void main() {\n"
      "  color = vec4(textColor, texture(text, TexCoords).r);\n"
      "}";
  shader_source_[MaterialSystem::kText] = {vert_shader, frag_shader, ""};
}

void GlStockShaders::ForwardShaders() {
  CommonShaderSnippets();

  ct::string vert_shader, frag_shader, geom_shader;

  vert_shader =
      vertex_header_ +

      "out VS_OUT{\n"
      "  vec3 WorldPos;\n"
      "  vec2 TexCoords;\n"
      "  vec3 Normal;\n"
      "} vs_out;\n\n"

      + matrix_input_ +

      "flat out int inst_id;\n"

      "void main() {\n"
      "  vec4 world_pos = world[gl_InstanceID] * vec4(position, 1.0);\n"
      "  gl_Position = view_proj * world_pos;\n"
      "  vs_out.WorldPos = world_pos.xyz;\n"
      "  vs_out.Normal = (world_inv_trans[gl_InstanceID]*vec4(normal, "
      "0.0)).xyz;\n"
      "  vs_out.TexCoords = texcoord;\n"
      "  inst_id = gl_InstanceID;\n"
      "}";

  frag_shader =
      "#version 330 core\n"
      "out vec4 FragColor;\n"
      "flat in int inst_id;\n"

      "in VS_OUT{\n"
      "  vec3 WorldPos;\n"
      "  vec2 TexCoords;\n"
      "  vec3 Normal;\n"
      "} fs_in;\n\n"

      "uniform vec3 albedo[100];\n"
      "uniform vec3 rme[100];\n"
      "uniform sampler2D albedo_tex;\n"
      "uniform sampler2D normal_tex;\n"
      "uniform sampler2D rma_tex;\n"

      "uniform float transp[100];\n"

      "uniform vec3 cam_pos;\n\n"

      + pbr_util_funcs_ + pbr_util_funcs_shadows_end_ +

      "void main()\n"
      "{\n"
      "  vec3 N = normalize(fs_in.Normal);\n"
      "  vec3 V = normalize(cam_pos - fs_in.WorldPos);\n\n"
      "  vec3 alb = albedo[inst_id];\n"
      "  vec3 rma = rme[inst_id];\n"
      "  float ao = rma.b;\n"

      "  vec3 F0 = vec3(0.04);\n"
      "  F0 = mix(F0, alb, rma.g);\n\n"
      "  vec3 Lo = PbrLighting(fs_in.WorldPos, V, N, F0, alb, rma.g, "
      "rma.r);\n"

      "  vec3 ambient = vec3(0.03) * alb + alb * ao;\n"
      "  vec3 color = ambient + Lo;\n\n"
      "  FragColor = vec4(color, transp[inst_id]);\n"
      "}";
  shader_source_[MaterialSystem::kPbrUntextured] = {vert_shader, frag_shader,
                                                    ""};

  vert_shader =
      vertex_header_ +

      "flat out int inst_id;\n"

      "uniform vec2 tex_scale[100];\n"
      "uniform vec2 tex_offset[100];\n"

      "out VS_OUT{\n"
      "  vec3 WorldPos;\n"
      "  vec2 TexCoords;\n"
      "  mat3 TBN;\n"
      "} vs_out;\n\n"

      + matrix_input_ +

      "void main() {\n"
      "  vec3 T = normalize((world_inv_trans[gl_InstanceID] * vec4(tangent, "
      "0.0f)).xyz);\n"
      "  vec3 N = normalize((world_inv_trans[gl_InstanceID] * vec4(normal, "
      "0.0f)).xyz);\n\n"
      "  vec3 B = normalize(cross(N, T));\n"

      "  vec4 world_pos = world[gl_InstanceID] * vec4(position, 1.0);\n"

      "  vs_out.WorldPos = world_pos.xyz;\n"

      "  vec2 scale_center = vec2(.5f, .5f);\n"
      "  vs_out.TexCoords = (texcoord - scale_center) * "
      "abs(tex_scale[gl_InstanceID]) + scale_center;\n"
      "  vs_out.TexCoords += tex_offset[gl_InstanceID];\n"

      "  if(tex_scale[gl_InstanceID][0] < 0.f){\n"
      "    vs_out.TexCoords[0] = 1.f - vs_out.TexCoords[0];\n"
      "  }\n"

      "  if(tex_scale[gl_InstanceID][1] < 0.f){\n"
      "    vs_out.TexCoords[1] = 1.f - vs_out.TexCoords[1];\n"
      "  }\n"

      "  vs_out.TBN = mat3(T, B, N);\n"
      "  gl_Position = view_proj * world_pos;\n"
      "  inst_id = gl_InstanceID;\n"
      "}";

  frag_shader =
      "#version 330 core\n"
      "out vec4 FragColor;\n"
      "flat in int inst_id;\n"

      "in VS_OUT{\n"
      "  vec3 WorldPos;\n"
      "  vec2 TexCoords;\n"
      "  mat3 TBN;\n"
      "} fs_in;\n\n"

      "uniform float transp[100];\n"
      "uniform vec3 albedo[100];\n"
      "uniform vec3 rme[100];\n"
      "uniform sampler2D albedo_tex;\n"
      "uniform sampler2D normal_tex;\n"
      "uniform sampler2D rma_tex;\n"

      "uniform vec3 cam_pos;\n"

      + pbr_util_funcs_ + pbr_util_funcs_shadows_end_ +

      "void main()\n"
      "{\n"
      "  vec3 V = normalize(cam_pos - fs_in.WorldPos);\n\n"

      "  vec3 N = texture(normal_tex, fs_in.TexCoords).rgb;\n"
      "  N = normalize(N * 2.0 - 1.0);\n"
      "  N = normalize(fs_in.TBN * N);\n"

      "  vec3 rma = rme[inst_id] + texture(rma_tex, fs_in.TexCoords).rgb;\n"
      "  vec3 alb = albedo[inst_id] * texture(albedo_tex, "
      "fs_in.TexCoords).rgb;\n"
      "  float metallic = rma.g;\n"
      "  float roughness = rma.r;\n"
      "  float ao = rma.b;\n\n"

      "  vec3 F0 = vec3(0.04);\n"
      "  F0 = mix(F0, alb, metallic);\n\n"
      "  vec3 Lo = vec3(0);\n"

      // PbrLighting(fs_in.WorldPos, V, N, F0, alb, metallic, "
      //"roughness);\n"

      "  vec3 ambient = vec3(0.03) * alb + alb * ao;\n"
      "  vec3 color = ambient + Lo;\n\n"
      "  FragColor = vec4(color, transp[inst_id]);\n"
      "}";
  shader_source_[MaterialSystem::kPbrTextured] = {vert_shader, frag_shader, ""};
}

void GlStockShaders::DeferredShaders() {
  CommonShaderSnippets();
  ct::string vert_shader =
      vertex_header_ +

      "out VS_OUT{\n"
      "  vec3 WorldPos;\n"
      "  vec2 TexCoords;\n"
      "  mat3 TBN;\n"
      "} vs_out;\n\n"

      + matrix_input_ +

      "flat out int inst_id;\n"
      "uniform vec2 tex_scale[100];\n"
      "uniform vec2 tex_offset[100];\n"

      "void main() {\n"
      "  vec3 T = normalize((world_inv_trans[gl_InstanceID] * vec4(tangent, "
      "0.0f)).xyz);\n"
      "  vec3 N = normalize((world_inv_trans[gl_InstanceID] * vec4(normal, "
      "0.0f)).xyz);\n\n"
      "  vec3 B = normalize(cross(N, T));\n"

      "  vec4 world_pos = world[gl_InstanceID] * vec4(position, 1.0);\n"

      "  vs_out.WorldPos = world_pos.xyz;\n"
      "  vec2 scale_center = vec2(.5f, .5f);\n"
      "  vs_out.TexCoords = (texcoord - scale_center) * "
      "abs(tex_scale[gl_InstanceID]) + scale_center;\n"
      "  vs_out.TexCoords += tex_offset[gl_InstanceID];\n"

      "  if(tex_scale[gl_InstanceID][0] < 0.f){\n"
      "    vs_out.TexCoords[0] = 1.f - vs_out.TexCoords[0];\n"
      "  }\n"

      "  if(tex_scale[gl_InstanceID][1] < 0.f){\n"
      "    vs_out.TexCoords[1] = 1.f - vs_out.TexCoords[1];\n"
      "  }\n"

      "  vs_out.TBN = mat3(T, B, N);\n"
      "  gl_Position = view_proj * world_pos;\n"
      "  inst_id = gl_InstanceID;\n"
      "}";

  ct::string frag_shader =
      "#version 330 core\n"
      "layout(location = 0) out vec3 g_position;\n"
      "layout(location = 1) out vec3 g_normal;\n"
      "layout(location = 2) out vec3 g_albedo;\n"
      "layout(location = 3) out vec3 g_rma;\n"

      "in VS_OUT{ \n"
      "  vec3 WorldPos;\n"
      "  vec2 TexCoords;\n"
      "  mat3 TBN;\n"
      "} fs_in;\n\n"

      "uniform vec3 albedo[100];\n"
      "uniform vec3 rme[100];\n"
      "uniform sampler2D albedo_tex;\n"
      "uniform sampler2D normal_tex;\n"
      "uniform sampler2D rma_tex;\n"

      "flat in int inst_id;\n"

      "void main() {\n"
      "  vec4 albedo_color = texture(albedo_tex, fs_in.TexCoords);\n"
      "  if(albedo_color.a < .5)\n"
      "  {\n"
      "    discard;\n"
      "  }\n"

      "  vec3 N = texture(normal_tex, fs_in.TexCoords).rgb;\n"
      "  N = normalize(N * 2.0 - 1.0);\n"
      "  N = normalize(fs_in.TBN * N);\n"

      "  g_position = fs_in.WorldPos;\n"
      "  g_normal = vec3(N*0.5+0.5);\n"

      "  g_albedo = albedo_color.rgb * albedo[inst_id];\n"
      "  g_rma = texture(rma_tex, fs_in.TexCoords).rgb + rme[inst_id];\n"
      "}";

  shader_source_[MaterialSystem::kPbrTextured] = {vert_shader, frag_shader, ""};

  vert_shader =
      vertex_header_ +

      "out VS_OUT{\n"
      "  vec3 WorldPos;\n"
      "  vec2 TexCoords;\n"
      "  vec3 Normal;\n"
      "} vs_out;\n\n"

      + matrix_input_ +

      "flat out int inst_id;\n"

      "void main() {\n"
      "  vec4 world_pos = world[gl_InstanceID] * vec4(position, 1.0);\n"
      "  vs_out.Normal = normalize((world_inv_trans[gl_InstanceID] * "
      "vec4(normal, "
      "0.0f)).xyz);\n"
      "  vs_out.WorldPos = world_pos.xyz;\n"
      "  vs_out.TexCoords = texcoord;\n"
      "  gl_Position = view_proj * world_pos;\n"
      "  inst_id = gl_InstanceID;\n"
      "}";

  frag_shader =
      "#version 330 core\n"
      "layout(location = 0) out vec3 g_position;\n"
      "layout(location = 1) out vec3 g_normal;\n"
      "layout(location = 2) out vec3 g_albedo;\n"
      "layout(location = 3) out vec3 g_rma;\n"

      "in VS_OUT{\n"
      "  vec3 WorldPos;\n"
      "  vec2 TexCoords;\n"
      "  vec3 Normal;\n"
      "} fs_in;\n\n"

      "uniform vec3 albedo[100];\n"
      "uniform vec3 rme[100];\n"

      "flat in int inst_id;\n"

      "void main() {\n"
      "  g_position = fs_in.WorldPos;\n"
      "  g_normal = normalize(fs_in.Normal)*0.5+0.5;\n"
      "  g_albedo = albedo[inst_id];\n"
      "  g_rma = rme[inst_id];\n"
      "}";

  shader_source_[MaterialSystem::kPbrUntextured] = {vert_shader, frag_shader,
                                                    ""};

  ct::string deferred_fragment_header =
      "#version 330 core\n"
      "out vec4 FragColor;\n"
      "flat in int inst_id;\n"

      "uniform sampler2D g_position;\n"
      "uniform sampler2D g_normal;\n"
      "uniform sampler2D g_albedo;\n"
      "uniform sampler2D g_rma;\n";

  ct::string deferred_fragment_start =
      "  vec3 frag_pos = texture(g_position, frag_coord).rgb;\n"
      "  vec3 N = normalize(texture(g_normal, frag_coord).rgb * 2 - 1);\n"
      "  vec3 V = normalize(cam_pos - frag_pos);\n\n"

      "  vec3 rma = texture(g_rma, frag_coord).rgb;\n"
      "  vec3 albedo = pow(texture(g_albedo, frag_coord).rgb, vec3(2.2));\n"
      "  float r = rma.r;\n"
      "  float m = rma.g;\n"
      "  float e = rma.b;\n\n"
      "  int i = inst_id;\n"

      "  vec3 F0 = vec3(0.04);\n"
      "  F0 = mix(F0, albedo, m);\n"
      "  vec3 Lo = vec3(0.0);\n";

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
      "uniform sampler2D ssao_tex;\n"

      "void main()\n"
      "{\n"
      "  vec2 frag_coord = TexCoords;\n"

      + deferred_fragment_start +

      "  float ambient_occ = clamp(texture(ssao_tex, frag_coord).r + 0.05, "
      "0.0, 1.0);\n"
      "  vec3 color = albedo * e + (vec3(0.05) * albedo) * "
      "ambient_occ;\n"
      "  FragColor = vec4(color, 1.0);\n"
      "}";
  shader_source_[MaterialSystem::kDeferredLightingAmbient] = {vert_shader,
                                                              frag_shader, ""};

  frag_shader =
      "#version 330 core\n"
      "out vec4 FragColor;\n"

      "in vec2 TexCoords;\n"

      "uniform sampler2D hdr_buffer;\n"
      "uniform sampler2D bloom_buffer;\n"
      "uniform float exposure;\n"
      "uniform float gamma;\n"
      "uniform float hue;\n"
      "uniform float saturation;\n"
      "uniform float brightness;\n"

      "vec3 rgb2hsv(vec3 c)\n"
      "{\n"
      "  vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);\n"
      "  vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));\n"
      "  vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));\n"

      "  float d = q.x - min(q.w, q.y);\n"
      "  float e = 1.0e-10;\n"
      "  return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), "
      "q.x);\n"
      "}\n"

      "vec3 hsv2rgb(vec3 c)\n"
      "{\n"
      "  vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);\n"
      "  vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);\n"
      "  return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);\n"
      "}\n"

      "float rand(vec2 co)\n"
      "{\n"
      "  return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);"
      "}\n"

      "void main() {\n"
      "  vec3 hdrColor = texture(hdr_buffer, TexCoords).rgb;\n"
      "  hdrColor += texture(bloom_buffer, TexCoords).rgb;\n"

      "  vec3 mapped = vec3(1.0) - exp(-hdrColor * exposure);\n"
      "  mapped = pow(mapped, vec3(1.0 / gamma));\n"

      "  vec3 hsvmod = rgb2hsv(mapped);\n"
      "  hsvmod.x *= hue;\n"
      "  hsvmod.y *= saturation;\n"
      "  hsvmod.z *= brightness;\n"

      // Add some noise to prevent banding
      "  float random_delta = (rand(TexCoords) * 2.0) - 1.0;\n"
      "  hsvmod.z -= 0.003 * random_delta;\n"
      "  mapped = hsv2rgb(hsvmod);\n"

      "  FragColor = vec4(mapped, 1.0);\n"
      "}";
  shader_source_[MaterialSystem::kTonemapGamma] = {vert_shader, frag_shader,
                                                   ""};
}

void GlStockShaders::CompileShaders(MaterialSystem *mat_mgr) {
  ForwardShaders();
  for (auto &p : shader_source_) {
    auto it = forward_shader_ids_.find(p.first);
    if (it == forward_shader_ids_.end()) {
      auto shader_command = AddShaderCommand(
          std::get<0>(p.second), std::get<1>(p.second), std::get<2>(p.second));
      issue_command(shader_command);
      forward_shader_ids_[p.first] = shader_command.ShaderId();
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
      issue_command(shader_command);
      deferred_shader_ids_[p.first] = shader_command.ShaderId();

      auto f_it = forward_shader_ids_.find(p.first);
      if (f_it != forward_shader_ids_.end())
        def_to_for_map_[shader_command.ShaderId()] = f_it->second;
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
