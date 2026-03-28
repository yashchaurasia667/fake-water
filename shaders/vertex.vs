#version 330 core

layout(location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

#define MAX_WAVES 12
uniform int u_numWaves;
uniform float u_amp[MAX_WAVES];
uniform float u_freq[MAX_WAVES];
uniform float u_speed[MAX_WAVES];
uniform float u_angle[MAX_WAVES];
uniform float u_time;

out vec3 Normal;
out vec3 FragPos;

void main() {
  float height = 0.0;
  float dhx = 0.0;
  float dhz = 0.0;

  for (int i = 0; i < u_numWaves; i++) {
    float phase = cos(u_angle[i]) * aPos.x + sin(u_angle[i]) * aPos.z;
    float args = u_freq[i] * phase + u_speed[i] * u_time;
    
    // Calculate the new height: A * (e^sin(args) - 1)
    float wave_height = exp(sin(args)) - 1.0;
    height += u_amp[i] * wave_height;

    // d/dx [e^sin(x)] = e^sin(x) * cos(x)
    float derivative = u_amp[i] * exp(sin(args)) * cos(args) * u_freq[i];

    // Apply the spatial partial derivatives (Fixed the cos/sin mapping here!)
    dhx += derivative * cos(u_angle[i]); 
    dhz += derivative * sin(u_angle[i]); 
  }

  vec3 displaced = vec3(aPos.x, height, aPos.z);
  vec4 worldPos = model * vec4(displaced, 1.0);
  FragPos = vec3(worldPos);

  gl_Position = projection * view * worldPos;

  vec3 Tx = vec3(1.0, dhx, 0.0);
  vec3 Tz = vec3(0.0, dhz, 1.0);
  Normal = normalize(cross(Tz, Tx));
}
