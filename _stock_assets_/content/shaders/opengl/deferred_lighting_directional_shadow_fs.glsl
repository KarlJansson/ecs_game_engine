#version 430 core

out vec4 FragColor;
in vec2 TexCoords;
flat in int inst_id;

uniform sampler2D g_position;
uniform sampler2D g_normal;
uniform sampler2D g_albedo;
uniform sampler2D g_rma;
uniform sampler2D g_depth;

uniform vec3 cam_pos;
uniform vec3 light_directions[50];
uniform vec3 light_color[50];
uniform mat4 world[3];
uniform sampler2DShadow depth_map[3];
uniform float cascade_depth[3];

const float PI = 3.14159265359;

float ShadowCalculation(vec4 fragPos, float ndotl, int i) {
  vec3 projCoords = fragPos.xyz / fragPos.w;
  projCoords = projCoords * 0.5 + 0.5;
  float currentDepth = projCoords.z;

  float bias;
  if (i == 0)
    bias = 0.001;
  else if (i == 1)
    bias = 0.001;
  else if (i == 2)
    bias = 0.01;

  // float bias = 0.001;//max(0.01 * (1.0 - ndotl), 0.0001);
  vec2 texelSize = 1.0 / textureSize(depth_map[i], 0);

  float shadow = 0.0;
  for (int x = -1; x <= 1; ++x) {
    for (int y = -1; y <= 1; ++y) {
      shadow += texture(
          depth_map[i],
          vec3(projCoords.xy + vec2(x, y) * texelSize, currentDepth - bias));
    }
  }
  shadow *= 0.25;
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
  vec3 L = normalize(-light_directions[i]);
  vec3 H = normalize(V + L);
  vec3 radiance = light_color[i];

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
  float depth = texture(g_depth, frag_coord).r;

  for (int l = 0; l < 3; ++l) {
    if (depth < cascade_depth[l] || l == 2) {
      vec4 light_pos = world[l] * vec4(frag_pos, 1.0);
      float shadow = ShadowCalculation(light_pos, NdotL, l);
      Lo = shadow * (kD * albedo / PI + brdf) * radiance * NdotL;
      break;
    }
  }

  FragColor = vec4(Lo, 1.0);
}
