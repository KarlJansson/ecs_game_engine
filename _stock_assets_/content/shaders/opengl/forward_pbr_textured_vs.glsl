#version 430 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec2 texcoord;

flat out int inst_id;

uniform vec2 tex_scale[100];
uniform vec2 tex_offset[100];
uniform mat4 world[100];
uniform mat4 world_inv_trans[100];
uniform mat4 view_proj;

out VS_OUT {
  vec3 WorldPos;
  vec2 TexCoords;
  mat3 TBN;
} vs_out;

void main() {
  vec3 T =
      normalize((world_inv_trans[gl_InstanceID] * vec4(tangent, 0.0f)).xyz);
  vec3 N = normalize((world_inv_trans[gl_InstanceID] * vec4(normal, 0.0f)).xyz);
  vec3 B = normalize(cross(N, T));

  vec4 world_pos = world[gl_InstanceID] * vec4(position, 1.0);

  vs_out.WorldPos = world_pos.xyz;

  vec2 scale_center = vec2(.5f, .5f);
  vs_out.TexCoords =
      (texcoord - scale_center) * abs(tex_scale[gl_InstanceID]) + scale_center;
  vs_out.TexCoords += tex_offset[gl_InstanceID];

  if (tex_scale[gl_InstanceID][0] < 0.f) {
    vs_out.TexCoords[0] = 1.f - vs_out.TexCoords[0];
  }

  if (tex_scale[gl_InstanceID][1] < 0.f) {
    vs_out.TexCoords[1] = 1.f - vs_out.TexCoords[1];
  }

  vs_out.TBN = mat3(T, B, N);
  gl_Position = view_proj * world_pos;
  inst_id = gl_InstanceID;
}
