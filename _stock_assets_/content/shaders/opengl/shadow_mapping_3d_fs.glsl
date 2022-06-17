#version 420 core

in vec4 FragPos;

uniform vec3 light_pos;
uniform float far_plane;

void main() {
  float lightDistance = length(FragPos.xyz - light_pos);
  lightDistance = lightDistance / far_plane;
  gl_FragDepth = lightDistance;
}
