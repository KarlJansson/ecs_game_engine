#version 420 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec2 texcoord;

out vec3 TexCoords;

uniform mat4 projection;
uniform mat4 view;

void main() {
  vec4 pos = projection * view * vec4(position, 1.0);
  gl_Position = pos.xyww;
  TexCoords = position.xyz;
}
