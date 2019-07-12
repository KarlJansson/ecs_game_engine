#version 430 core

out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D hdr_buffer;
uniform sampler2D bloom_buffer;
uniform float exposure;
uniform float gamma;
uniform float hue;
uniform float saturation;
uniform float brightness;

vec3 rgb2hsv(vec3 c) {
  vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
  vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
  vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

  float d = q.x - min(q.w, q.y);
  float e = 1.0e-10;
  return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv2rgb(vec3 c) {
  vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
  vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
  return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

float rand(vec2 co) {
  return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

void main() {
  vec3 hdrColor = texture(hdr_buffer, TexCoords).rgb;
  hdrColor += texture(bloom_buffer, TexCoords).rgb;

  vec3 mapped = vec3(1.0) - exp(-hdrColor * exposure);
  mapped = pow(mapped, vec3(1.0 / gamma));

  vec3 hsvmod = rgb2hsv(mapped);
  hsvmod.x *= hue;
  hsvmod.y *= saturation;
  hsvmod.z *= brightness;

  // Add some noise to prevent banding
  float random_delta = (rand(TexCoords) * 2.0) - 1.0;
  hsvmod.z -= 0.003 * random_delta;
  mapped = hsv2rgb(hsvmod);

  FragColor = vec4(mapped, 1.0);
}
