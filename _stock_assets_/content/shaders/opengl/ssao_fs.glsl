#version 430 core

out float FragColor;
in vec2 TexCoords;

uniform sampler2D g_position;
uniform sampler2D g_normal;
uniform sampler2D tex_noise;

uniform vec3 samples[64];
uniform mat4 projection;
uniform mat4 view;

uniform int kernelSize;
uniform float radius;
uniform float bias;
uniform vec2 noise_scale;

void main() {
  vec3 fragPos = (view * vec4(texture(g_position, TexCoords).xyz, 1)).xyz;
  vec3 normal =
      (view * vec4(texture(g_normal, TexCoords).rgb * 2 - 1.0, 0)).xyz;
  normal = normalize(normal);
  vec3 randomVec = texture(tex_noise, TexCoords * noise_scale).xyz * 2 - 1.0;
  vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
  vec3 bitangent = cross(normal, tangent);
  mat3 TBN = mat3(tangent, bitangent, normal);
  float occlusion = 0.0;
  for (int i = 0; i < kernelSize; ++i) {
    vec3 kernel_sample = TBN * samples[i];
    kernel_sample = fragPos + kernel_sample * radius;

    vec4 offset = vec4(kernel_sample, 1.0);
    offset = projection * offset;
    offset.xyz /= offset.w;
    offset.xyz = offset.xyz * 0.5 + 0.5;
    float sampleDepth = (view * vec4(texture(g_position, offset.xy))).z;

    float rangeCheck =
        smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
    occlusion +=
        (sampleDepth >= kernel_sample.z + bias ? 1.0 : 0.0) * rangeCheck;
  }
  occlusion = 1.0 - (occlusion / kernelSize);
  FragColor = occlusion;
}
