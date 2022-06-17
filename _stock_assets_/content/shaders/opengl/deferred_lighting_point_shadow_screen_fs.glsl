#version 420 core

out vec4 FragColor;
in vec2 TexCoords;
flat in int inst_id;

uniform sampler2D g_position;
uniform sampler2D g_normal;
uniform sampler2D g_albedo;
uniform sampler2D g_rma;
uniform sampler2D g_depth;

uniform vec3 cam_pos;
uniform samplerCube depth_map[5];
uniform float far_plane[5];
uniform vec3 light_position[50];
uniform vec3 light_coeff[50];
uniform float light_radius[50];
uniform vec3 light_color[50];

const float PI = 3.14159265359;

float ShadowCalculation(vec3 fragPos, int i) {
  vec3 fragToLight = fragPos - light_position[i];
  float currentDepth = length(fragToLight);
  float bias = 0.05;
  float shadow = 0.0;
  float samples = 4.0;
  float offset = 0.1;
  for (float x = -offset; x < offset; x += offset / (samples * 0.5)) {
    for (float y = -offset; y < offset; y += offset / (samples * 0.5)) {
      for (float z = -offset; z < offset; z += offset / (samples * 0.5)) {
        float closestDepth =
            texture(depth_map[i], fragToLight + vec3(x, y, z)).r;
        closestDepth *= far_plane[i];
        if (currentDepth - bias > closestDepth) shadow += 1.0;
      }
    }
  }
  shadow /= (samples * samples * samples);

  return shadow;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
  return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness) {
  float a = roughness * roughness;
  float a2 = a * a;
  float NdotH = max(dot(N, H), 0.0);
  float NdotH2 = NdotH * NdotH;

  float nom = a2;
  float denom = (NdotH2 * (a2 - 1.0) + 1.0);
  denom = PI * denom * denom;

  return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
  float r = (roughness + 1.0);
  float k = (r * r) / 8.0;

  float nom = NdotV;
  float denom = NdotV * (1.0 - k) + k;

  return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
  float NdotV = max(dot(N, V), 0.0);
  float NdotL = max(dot(N, L), 0.0);
  float ggx2 = GeometrySchlickGGX(NdotV, roughness);
  float ggx1 = GeometrySchlickGGX(NdotL, roughness);

  return ggx1 * ggx2;
}

void main() {
  vec2 frag_coord = TexCoords;
  vec3 frag_pos = texture(g_position, frag_coord).rgb;
  vec3 N = normalize(texture(g_normal, frag_coord).rgb * 2 - 1);

  vec3 V = normalize(cam_pos - frag_pos);

  vec3 rma = texture(g_rma, frag_coord).rgb;
  vec3 albedo = pow(texture(g_albedo, frag_coord).rgb, vec3(2.2));

  float r = clamp(rma.r, 0.05, 1.0);
  float m = rma.g;
  float e = rma.b;
  int i = inst_id;

  vec3 F0 = vec3(0.04);
  F0 = mix(F0, albedo, m);
  vec3 Lo = vec3(0.0);
  vec3 L = normalize(light_position[i] - frag_pos);
  vec3 H = normalize(V + L);
  float distance = length(light_position[i] - frag_pos);
  float attenuation = 1.0 / (light_coeff[i].x + light_coeff[i].y * distance +
                             light_coeff[i].z * (distance * distance));
  attenuation *= clamp(1. - ((1. / light_radius[i]) * distance), 0., 1.);
  vec3 radiance = light_color[i] * clamp(attenuation, 0.0, 1.0);

  float NDF = DistributionGGX(N, H, r);
  float G = GeometrySmith(N, V, L, r);
  vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
  vec3 kS = F;
  vec3 kD = vec3(1.0) - kS;
  kD *= 1.0 - m;

  vec3 nominator = NDF * G * F;
  float denominator = max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001;
  vec3 brdf = nominator / denominator;

  float NdotL = max(dot(N, L), 0.0);
  float shadow = 1.0 - ShadowCalculation(frag_pos, i);
  Lo = shadow * (kD * albedo / PI + brdf) * radiance * NdotL;
  FragColor = vec4(Lo, 1.0);
}
