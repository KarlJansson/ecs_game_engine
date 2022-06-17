#version 420 core

layout(location = 0) in vec4 random;
layout(location = 1) in vec3 position;
layout(location = 2) in vec3 velocity;
layout(location = 3) in vec2 corners;
layout(location = 4) in float start_time;

out vec2 tex_coord;
out vec4 color;

uniform mat4 view;
uniform mat4 projection;
uniform vec2 viewport_scale;

uniform float current_time;
uniform float duration;
uniform float duration_randomness;
uniform vec3 gravity;
uniform float end_velocity;
uniform vec4 min_color;
uniform vec4 max_color;

uniform vec2 rotate_speed;
uniform vec2 start_size;
uniform vec2 end_size;

vec4 ComputeParticlePosition(vec3 pos, vec3 velocity, float age,
                             float normalizedAge) {
  float startVelocity = length(velocity);
  float endVelocity = startVelocity * end_velocity;
  float velocityIntegral =
      startVelocity * normalizedAge +
      (endVelocity - startVelocity) * normalizedAge * normalizedAge / 2;
  pos += normalize(velocity) * velocityIntegral * duration;
  pos += gravity * age * normalizedAge;
  return projection * view * vec4(pos, 1.f);
}

float ComputeParticleSize(float randomValue, float normalizedAge) {
  float startSize = mix(start_size.x, start_size.y, randomValue);
  float endSize = mix(end_size.x, end_size.y, randomValue);
  float size = mix(startSize, endSize, normalizedAge);
  return size * projection[1][1];
}

vec4 ComputeParticleColor(float randomValue, float normalizedAge) {
  vec4 color = mix(min_color, max_color, randomValue);
  color = mix(color, max_color, normalizedAge);
  color.a *=
      normalizedAge * (1.0f - normalizedAge) * (1.0f - normalizedAge) * 6.7;
  return color;
}

mat2 ComputeParticleRotation(float randomValue, float age) {
  float rotateSpeed = mix(rotate_speed.x, rotate_speed.y, randomValue);
  float rotation = rotateSpeed * age;
  float c = cos(rotation);
  float s = sin(rotation);
  return mat2(c, -s, s, c);
}

void main() {
  float age = current_time - start_time;
  age *= 1.0f + random.x * duration_randomness;
  float normalized_age = clamp(age / duration, 0.f, 1.f);
  vec4 p = ComputeParticlePosition(position, velocity, age, normalized_age);
  float size = ComputeParticleSize(random.y, normalized_age);
  mat2 rotation = ComputeParticleRotation(random.w, age);
  p.xy += corners * rotation * size * viewport_scale;

  color = ComputeParticleColor(random.z, normalized_age);
  tex_coord = (corners + 1) / 2;

  gl_Position = p;
}
