#version 430 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec2 texcoord;

uniform mat4 world[50];
uniform mat4 view_proj;

flat out int inst_id;

void main() {
  gl_Position = view_proj * (world[gl_InstanceID] * vec4(position, 1.0));
  inst_id = gl_InstanceID;
}
