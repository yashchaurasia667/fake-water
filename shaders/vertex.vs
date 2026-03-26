#version 330 core

layout(location = 0) in vec3 aPos;
// layout(location = 1) in vec2 aTexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

#define MAX_WAVES 4
uniform int u_numWaves;
uniform float u_amp[MAX_WAVES];
uniform float u_freq[MAX_WAVES];
uniform float u_speed[MAX_WAVES];
uniform float u_time;

void main() {
  float height = 0.0;
  for (int i = 0; i < u_numWaves; i++) {
    // float phase = cos(u_angle[i]) * aPos.x + sin(u_angle[i]) * aPos.z;
    // height += u_amp[i] * sin(u_freq[i] * phase + u_speed[i] * u_time);
    height += u_amp[i] * sin((u_freq[i] * (aPos.x * aPos.z)) + (u_speed[i] * u_time));
    // h += waves[i].amp * sin((waves[i].freq * (x + y)) + (waves[i].speed * (float)std::time(0)));
  }
  vec3 displaced = vec3(aPos.x, height, aPos.z);
  gl_Position = projection * view * model * vec4(displaced, 1.0);

  // gl_Position = projection * view * model * vec4(aPos, 1.0);
}
