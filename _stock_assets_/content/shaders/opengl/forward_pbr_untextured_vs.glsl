#version 420 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec2 texcoord;

out VS_OUT {
  vec3 WorldPos;
  vec2 TexCoords;
  vec3 Normal;
} vs_out;

uniform mat4 world[100];
uniform mat4 world_inv_trans[100];
uniform mat4 view_proj;

flat out int inst_id;

void main() {
  vec4 world_pos = world[gl_InstanceID] * vec4(position, 1.0);
  gl_Position = view_proj * world_pos;
  vs_out.WorldPos = world_pos.xyz;
  vs_out.Normal = (world_inv_trans[gl_InstanceID] * vec4(normal, 0.0)).xyz;
  vs_out.TexCoords = texcoord;
  inst_id = gl_InstanceID;
}
