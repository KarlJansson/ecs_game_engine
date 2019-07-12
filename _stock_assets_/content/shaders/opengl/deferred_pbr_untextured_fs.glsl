#version 430 core

layout(location = 0) out vec3 g_position;
layout(location = 1) out vec3 g_normal;
layout(location = 2) out vec3 g_albedo;
layout(location = 3) out vec3 g_rma;

in VS_OUT {
  vec3 WorldPos;
  vec2 TexCoords;
  vec3 Normal;
} fs_in;

uniform vec3 albedo[100];
uniform vec3 rme[100];

flat in int inst_id;

void main() {
  g_position = fs_in.WorldPos;
  g_normal = normalize(fs_in.Normal) * 0.5 + 0.5;
  g_albedo = albedo[inst_id];
  g_rma = rme[inst_id];
}
