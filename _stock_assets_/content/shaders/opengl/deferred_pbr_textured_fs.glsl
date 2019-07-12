#version 430 core

layout(location = 0) out vec3 g_position;
layout(location = 1) out vec3 g_normal;
layout(location = 2) out vec3 g_albedo;
layout(location = 3) out vec3 g_rma;

in VS_OUT {
  vec3 WorldPos;
  vec2 TexCoords;
  mat3 TBN;
} fs_in;

uniform vec3 albedo[100];
uniform vec3 rme[100];
uniform sampler2D albedo_tex;
uniform sampler2D normal_tex;
uniform sampler2D rma_tex;

flat in int inst_id;

void main() {
  vec4 albedo_color = texture(albedo_tex, fs_in.TexCoords);
  if (albedo_color.a < .5) {
    discard;
  }

  vec3 N = texture(normal_tex, fs_in.TexCoords).rgb;
  N = normalize(N * 2.0 - 1.0);
  N = normalize(fs_in.TBN * N);

  g_position = fs_in.WorldPos;
  g_normal = vec3(N * 0.5 + 0.5);

  g_albedo = albedo_color.rgb * albedo[inst_id];
  g_rma = texture(rma_tex, fs_in.TexCoords).rgb + rme[inst_id];
}
