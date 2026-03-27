#version 330 core 
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

// L0(x, w0, Lambda, t) = fr * Li * cos(theta)
uniform vec3 u_lightDir;
uniform vec3 u_lightColor;
uniform vec3 u_waterColor;
uniform vec3 u_viewPos;
uniform int u_shininess;

float specStrength = 1.5;
vec3 ambient = vec3(0.2);

void main() {
  vec3 viewDir = normalize(u_viewPos - FragPos);
  vec3 halfway = normalize(viewDir + u_lightDir);
  vec3 norm = normalize(Normal);

  float spec = pow(max(dot(norm, halfway), 0.0), u_shininess);
  vec3 specular = specStrength * spec * u_lightColor;

  float diff = max(dot(norm, u_lightDir), 0.0);
  vec3 diffuse = diff * u_lightColor;

  vec3 result = (ambient + diffuse + specular) * u_waterColor;
  FragColor = vec4(result, 1.0);
}
