#version 420 core

out vec4 FragColor;
flat in int inst_id;

in VS_OUT {
  vec3 WorldPos;
  vec2 TexCoords;
  mat3 TBN;
} fs_in;

uniform float transp[100];
uniform vec3 albedo[100];
uniform vec3 rme[100];
uniform sampler2D albedo_tex;
uniform sampler2D normal_tex;
uniform sampler2D rma_tex;
uniform vec3 cam_pos;
uniform int nr_lights;
uniform vec3 light_position[4];
uniform vec3 light_color[4];
uniform samplerCube depth_map[4];
uniform float far_plane[4];
uniform int shadow_caster[4];

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

vec3 PbrLighting(vec3 frag_pos, vec3 V, vec3 N, vec3 F0, vec3 a, float m,
                 float r) {
  vec3 Lo = vec3(0.0);
  for (int i = 0; i < nr_lights; ++i) {
    vec3 L = normalize(light_position[i] - frag_pos);
    vec3 H = normalize(V + L);
    float distance = length(light_position[i] - frag_pos);
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
    float shadow = 1.0;
    if (shadow_caster[i] == 1) shadow = 1.0 - ShadowCalculation(frag_pos, i);
    Lo += shadow * (kD * a / PI + brdf) * radiance * NdotL;
  }
  return Lo;
}

void main() {
  vec3 V = normalize(cam_pos - fs_in.WorldPos);

  vec3 N = texture(normal_tex, fs_in.TexCoords).rgb;
  N = normalize(N * 2.0 - 1.0);
  N = normalize(fs_in.TBN * N);

  vec3 rma = rme[inst_id] + texture(rma_tex, fs_in.TexCoords).rgb;
  vec3 alb = albedo[inst_id] * texture(albedo_tex, fs_in.TexCoords).rgb;
  float metallic = rma.g;
  float roughness = rma.r;
  float ao = rma.b;

  vec3 F0 = vec3(0.04);
  F0 = mix(F0, alb, metallic);
  vec3 Lo = vec3(0);

  // PbrLighting(fs_in.WorldPos, V, N, F0, alb, metallic, roughness);

  vec3 ambient = vec3(0.03) * alb + alb * ao;
  vec3 color = ambient + Lo;
  FragColor = vec4(color, transp[inst_id]);
}
