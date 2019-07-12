#version 430 core

in vec2 tex_coord;
in vec4 color;

uniform sampler2D particle_texture;
uniform sampler2D g_depth;
uniform vec2 screen_dim;

out vec4 FragColor;

void main() {
  vec2 frag_coord = gl_FragCoord.xy / screen_dim;
  float depth = texture(g_depth, frag_coord).r;
  vec4 out_color = texture(particle_texture, tex_coord) * color;
  out_color.a *=
      clamp(abs(depth - gl_FragCoord.z) / gl_FragCoord.w, 0.f, 1.f) * 10.f;
  FragColor = out_color;
}
