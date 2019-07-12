#version 430 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec2 texcoord;

uniform mat4 shadow_matrices[3];
uniform mat4 world[200];

void main() {
  gl_Position =
      shadow_matrices[0] * (world[gl_InstanceID] * vec4(position, 1.0f));
}
