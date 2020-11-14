#version 430 core

out vec4 FragColor;
in vec2 TexCoords;
flat in int inst_id;

uniform sampler2D g_position;
uniform sampler2D g_normal;
uniform sampler2D g_albedo;
uniform sampler2D g_rma;

uniform vec3 cam_pos;
uniform sampler2D ssao_tex;

void main() {
  vec2 frag_coord = TexCoords;
  vec3 frag_pos = texture(g_position, frag_coord).rgb;
  vec3 N = normalize(texture(g_normal, frag_coord).rgb * 2 - 1);
  vec3 V = normalize(cam_pos - frag_pos);

  vec3 rma = texture(g_rma, frag_coord).rgb;
  vec3 albedo = pow(texture(g_albedo, frag_coord).rgb, vec3(2.2));
  float r = rma.r;
  float m = rma.g;
  float e = rma.b;
  int i = inst_id;

  vec3 F0 = vec3(0.04);
  F0 = mix(F0, albedo, m);
  vec3 Lo = vec3(0.0);

  float ambient_occ = clamp(texture(ssao_tex, frag_coord).r + 0.05, 0.0, 1.0);
  vec3 color = albedo * e + (vec3(0.05) * albedo) * ambient_occ;

  FragColor = vec4(color, 1.0);
}
