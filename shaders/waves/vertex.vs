#version 330 core

layout(location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

#define MAX_WAVES 160
uniform int u_numWaves;
uniform float u_amp;
uniform float u_freq;
uniform float u_speed[MAX_WAVES];
uniform float u_angle[MAX_WAVES];
uniform float u_amp_coeff;
uniform float u_freq_coeff;
uniform float u_time;

out vec3 Normal;
out vec3 FragPos;

void main() {
  float height = 0.0;
  float dhx = 0.0;
  float dhz = 0.0;

  float curr_amp = u_amp;
  float curr_freq= u_freq;
  float prev_derivative_x = 0;
  float prev_derivative_z = 0;

  for (int i = 0; i < u_numWaves; i++) {
    float phase = cos(u_angle[i]) * (aPos.x + prev_derivative_x) + sin(u_angle[i]) * (aPos.z + prev_derivative_z);
    float args = curr_freq * phase + u_speed[i] * u_time;
    
    // amp * e^(sin(fx + st)-1)
    float wave_height = exp(sin(args) - 1.0);
    height += curr_amp * wave_height;

    // d/dx [e^sin(x)] = e^sin(x) * cos(x)
    float derivative = curr_amp * exp(sin(args) - 1.0) * cos(args) * curr_freq;

    // Apply the spatial partial derivatives (Fixed the cos/sin mapping here!)
    dhx += derivative * cos(u_angle[i]); 
    dhz += derivative * sin(u_angle[i]); 

    curr_amp = curr_amp * u_amp_coeff;
    curr_freq = curr_freq * u_freq_coeff;
    prev_derivative_x = dhx;
    prev_derivative_z = dhz;
  }

  vec3 displaced = vec3(aPos.x, height, aPos.z);
  vec4 worldPos = model * vec4(displaced, 1.0);
  FragPos = vec3(worldPos);

  gl_Position = projection * view * worldPos;

  vec3 Tx = vec3(1.0, dhx, 0.0);
  vec3 Tz = vec3(0.0, dhz, 1.0);
  Normal = normalize(cross(Tz, Tx));
}
