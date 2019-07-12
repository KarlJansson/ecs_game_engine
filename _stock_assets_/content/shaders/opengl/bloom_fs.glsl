#version 430 core

in vec2 TexCoords;
out vec4 color;

uniform sampler2D hdr_buffer;
uniform sampler2D g_rma;

void main() {
  float emissive_color = texture(g_rma, TexCoords).b;
  vec4 hdr_color = texture(hdr_buffer, TexCoords);
  float brightness = dot(hdr_color.rgb, vec3(0.2126, 0.7152, 0.0722));
  if (brightness > 1.0 || emissive_color > 0.5)
    color = vec4(vec3(emissive_color) - exp(-hdr_color.rgb), .5);
  else
    discard;
}
