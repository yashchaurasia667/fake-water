#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui/imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <iostream>
#include <string>
#include <random>
#include <vector>

#include <shader.h>
#include <camera.h>
#include <utils.h>
#include <vertexArray.h>
#include <vertexBuffer.h>
#include <vertexBufferLayout.h>
#include <indexBuffer.h>

unsigned int scr_width = 1280, scr_height = 720;
Camera camera(glm::vec3(345.6f, 122.3f, 80.5f), 70.0f, 0.1f, 30.0f);
bool camera_movement = false;
bool framebufferResized = false;

struct SceneFBO
{
  unsigned int id;
  unsigned int colorTex;
  unsigned int depthTex; // changed: renderbuffer -> texture so the screen shader can sample it
};

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution dis(0.0f, 1.0f);

void framebufferSizeCallback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow *window);
void cursorPosCallback(GLFWwindow *window, double xposin, double yposin);
void scrollCallback(GLFWwindow *window, double xoffset, double yoffset);
void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods);

SceneFBO createFBO(unsigned int width, unsigned int height);
void deleteFBO(SceneFBO &fbo);
unsigned int loadCubemap(const std::string &basePath, const std::vector<std::string> &faces);

glm::vec3 *initSumOfSines(int n_waves, float max_amp, float max_freq, float max_speed);
float *initRandoms(int n_waves);
float *initAngles(int n_waves, float spread_degrees, float wind_dir_degrees);

int main()
{
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  GLFWwindow *window = glfwCreateWindow(scr_width, scr_height, "TITLE_HERE", NULL, NULL);
  if (window == nullptr)
  {
    std::cout << "Failed to create a GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
  {
    std::cout << "Failed to initialize opengl function pointers" << std::endl;
    return -1;
  }
  glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
  glfwSetMouseButtonCallback(window, mouseButtonCallback);
  glfwSetScrollCallback(window, scrollCallback);
  glEnable(GL_DEPTH_TEST);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

  ImGui_ImplGlfw_InitForOpenGL(window, false);
  ImGui_ImplOpenGL3_Init("#version 330 core");

  {
    Shader shader("../shaders/waves/vertex.vs", "../shaders/waves/fragment.fs");
    Shader screenShader("../shaders/screen/vertex.vs", "../shaders/screen/fragment.fs");
    Shader skyboxShader("../shaders/skybox/vertex.vs", "../shaders/skybox/fragment.fs");

    // Wave geometry
    int num_edge_vertices = 1000;
    float interval = 0.4f;
    float *vertex_pos = new float[num_edge_vertices * num_edge_vertices * 3];
    int indx = 0;
    for (int z = 0; z < num_edge_vertices; z++)
    {
      for (int x = 0; x < num_edge_vertices; x++)
      {
        vertex_pos[indx++] = (float)x * interval;
        vertex_pos[indx++] = 0.0f;
        vertex_pos[indx++] = (float)z * interval;
      }
    }

    int num_indices = (num_edge_vertices - 1) * (num_edge_vertices - 1) * 6;
    unsigned int *indices = new unsigned int[num_indices];
    int idx = 0;
    for (int z = 0; z < num_edge_vertices - 1; z++)
    {
      for (int x = 0; x < num_edge_vertices - 1; x++)
      {
        int v0 = z * num_edge_vertices + x;
        int v1 = v0 + 1;
        int v2 = v0 + num_edge_vertices;
        int v3 = v2 + 1;
        indices[idx++] = v0;
        indices[idx++] = v1;
        indices[idx++] = v2;
        indices[idx++] = v1;
        indices[idx++] = v3;
        indices[idx++] = v2;
      }
    }

    VertexArray vao;
    VertexBuffer vbo(num_edge_vertices * num_edge_vertices * 3 * sizeof(float), vertex_pos, GL_STATIC_DRAW);
    VertexBufferLayout layout;
    layout.push<float>(3);
    vao.addBuffer(vbo, layout);

    vao.bind();
    IndexBuffer ebo;
    ebo.bind();
    ebo.setData(num_indices * sizeof(unsigned int), indices, GL_STATIC_DRAW);
    vao.unbind();

    // FBO
    SceneFBO sceneFBO = createFBO(scr_width, scr_height);

    // Load cubemap
    std::vector<std::string> faces = {
        "px.png", "nx.png",
        "py.png", "ny.png",
        "pz.png", "nz.png"};
    unsigned int cubemapTexture = loadCubemap("../assets/skybox_night/cubemap", faces);

    // imgui controlled wave params
    int num_waves = 32;
    float min_speed = 0.4f, max_speed = 2.5f, spread = 70.0f, wind_dir = 0.0f;
    float *speed = initRandoms(num_waves);
    float *angles = initAngles(num_waves, spread, wind_dir);
    float amp = dis(gen), freq = dis(gen), amp_coeff = 0.5f + dis(gen) * 0.25f;

    // imgui controlled light params
    float ambient = 0.19f;
    glm::vec3 light_dir = glm::vec3(-0.78f, 0.84, 0.60);
    glm::vec3 light_color = glm::vec3(1.0f);
    glm::vec3 water_color = glm::vec3(0.08f, 0.33f, 0.51f);

    // fog params
    float fog_density = 0.0024f;
    float horizon_band = 0.08f;                           // tune this: smaller = sharper horizon line, larger = wider blend
    glm::vec3 fog_color = glm::vec3(0.38f, 0.58f, 0.72f); // match your skybox horizon colour

    glClearColor(0.4, 0.4, 0.4, 1.0);

    while (!glfwWindowShouldClose(window))
    {
      camera.updateFrame();

      if (framebufferResized)
      {
        deleteFBO(sceneFBO);
        sceneFBO = createFBO(scr_width, scr_height);
        framebufferResized = false;
      }

      glfwPollEvents();
      processInput(window);

      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();

      {
        ImGui::Begin("Wave params");
        ImGui::SliderInt("num waves", &num_waves, 16, 160);
        ImGui::SliderFloat("amp", &amp, 0.0f, 10.0f, "%.2f");
        ImGui::SliderFloat("amp_coeff", &amp_coeff, 0.0f, 1.0f, "%.2f");
        ImGui::SliderFloat("freq", &freq, 0.0f, 10.0f, "%.2f");
        ImGui::SliderFloat("spread", &spread, 0.0f, 360.0f, "%.2f");
        ImGui::SliderFloat("wind_dir", &wind_dir, 0.0f, 360.0f, "%.2f");
        ImGui::SliderFloat("max_speed", &max_speed, 0.0f, 20.0f, "%.2f");
        ImGui::SliderFloat("min_speed", &min_speed, 0.0f, 10.0f, "%.2f");
        if (ImGui::Button("Init wave"))
        {
          std::cout << "Re-initializing wave" << std::endl;
          delete[] speed;
          delete[] angles;
          speed = initRandoms(num_waves);
          angles = initAngles(num_waves, spread, wind_dir);
        }
        ImGui::End();
      }
      {
        ImGui::Begin("Light params");
        ImGui::SliderFloat("ambient", &ambient, 0.0f, 1.0f, "%.2f");
        if (ImGui::CollapsingHeader("light direction"))
        {
          ImGui::SliderFloat("x", &light_dir.x, -1.0f, 1.0f, "%.2f");
          ImGui::SliderFloat("y", &light_dir.y, -1.0f, 1.0f, "%.2f");
          ImGui::SliderFloat("z", &light_dir.z, -1.0f, 1.0f, "%.2f");
        }
        if (ImGui::CollapsingHeader("light color"))
        {
          ImGui::SliderFloat("r", &light_color.x, 0.0f, 1.0f, "%.2f");
          ImGui::SliderFloat("g", &light_color.y, 0.0f, 1.0f, "%.2f");
          ImGui::SliderFloat("b", &light_color.z, 0.0f, 1.0f, "%.2f");
        }
        if (ImGui::CollapsingHeader("water color"))
        {
          ImGui::SliderFloat("wr", &water_color.x, 0.0f, 1.0f, "%.2f");
          ImGui::SliderFloat("wg", &water_color.y, 0.0f, 1.0f, "%.2f");
          ImGui::SliderFloat("wb", &water_color.z, 0.0f, 1.0f, "%.2f");
        }
        ImGui::End();
      }
      {
        ImGui::Begin("Fog params");
        ImGui::SliderFloat("density", &fog_density, 0.0f, 0.1f, "%.4f");
        ImGui::SliderFloat("horizon band", &horizon_band, 0.0f, 0.5f, "%.3f");
        if (ImGui::CollapsingHeader("fog color"))
        {
          ImGui::SliderFloat("fr", &fog_color.x, 0.0f, 1.0f, "%.3f");
          ImGui::SliderFloat("fg", &fog_color.y, 0.0f, 1.0f, "%.3f");
          ImGui::SliderFloat("fb", &fog_color.z, 0.0f, 1.0f, "%.3f");
        }
        ImGui::End();
      }

      glm::mat4 model = glm::mat4(1.0f);
      glm::mat4 view = camera.getViewMatrix();
      glm::mat4 projection = glm::perspective(camera.getFov(),
                                              (float)scr_width / (float)scr_height,
                                              0.1f, 1000.0f);

      // Pass 1: render waves into the FBO (unchanged)
      glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO.id);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      shader.bind();
      shader.setMat4("model", model);
      shader.setMat4("view", view);
      shader.setMat4("projection", projection);
      shader.setInt("u_numWaves", num_waves);
      shader.setFloat("u_amp", amp);
      shader.setFloat("u_freq", freq);
      shader.setFloat("u_amp_coeff", amp_coeff);
      shader.setFloat("u_freq_coeff", 2.0f - amp_coeff);
      for (int i = 0; i < num_waves; i++)
      {
        shader.setFloat(("u_speed[" + std::to_string(i) + "]").c_str(),
                        min_speed + speed[i] * max_speed);
        shader.setFloat(("u_angle[" + std::to_string(i) + "]").c_str(), angles[i]);
      }
      shader.setFloat("u_time", (float)glfwGetTime());
      shader.setFloat("u_ambient", ambient);
      shader.setVec3("u_lightDir", light_dir);
      shader.setVec3("u_lightColor", light_color);
      shader.setVec3("u_waterColor", water_color);
      shader.setVec3("u_viewPos", camera.getPos());
      shader.setInt("u_shininess", 128);

      vao.bind();
      ebo.bind();
      glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_INT, 0);

      // Pass 2: blit depth, draw fullscreen quad with fog
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      glBindFramebuffer(GL_READ_FRAMEBUFFER, sceneFBO.id);
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
      glBlitFramebuffer(0, 0, scr_width, scr_height,
                        0, 0, scr_width, scr_height,
                        GL_DEPTH_BUFFER_BIT, GL_NEAREST);
      glBindFramebuffer(GL_FRAMEBUFFER, 0);

      glDisable(GL_DEPTH_TEST);
      glDepthMask(GL_FALSE);

      // Precompute inverse matrices once per frame
      glm::mat4 invProjection = glm::inverse(projection);
      glm::mat3 invViewRot = glm::transpose(glm::mat3(view)); // view rotation is orthogonal so inverse == transpose

      screenShader.bind();
      screenShader.setInt("u_screenTexture", 0);
      screenShader.setInt("u_depthTexture", 1);
      screenShader.setFloat("u_near", 0.1f);
      screenShader.setFloat("u_far", 1000.0f);
      screenShader.setFloat("u_fogDensity", fog_density);
      screenShader.setFloat("u_horizonBand", horizon_band);
      screenShader.setVec3("u_fogColor", fog_color);
      screenShader.setMat4("u_invProjection", invProjection);
      screenShader.setMat4("u_invViewRot", invViewRot);

      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, sceneFBO.colorTex);
      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D, sceneFBO.depthTex);

      renderQuad();

      glDepthMask(GL_TRUE);
      glEnable(GL_DEPTH_TEST);

      // Pass 3: skybox (unchanged)
      glDepthFunc(GL_LEQUAL);
      skyboxShader.bind();
      glm::mat4 skyView = glm::mat4(glm::mat3(view));
      skyboxShader.setMat4("view", skyView);
      skyboxShader.setMat4("projection", projection);
      skyboxShader.setInt("u_skybox", 0);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
      renderCube();
      glDepthFunc(GL_LESS);

      ImGui::Render();
      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

      glfwSwapBuffers(window);
    }

    delete[] vertex_pos;
    delete[] indices;
    delete[] speed;
    delete[] angles;

    deleteFBO(sceneFBO);
    glDeleteTextures(1, &cubemapTexture);
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwTerminate();
  return 0;
}

void framebufferSizeCallback(GLFWwindow *window, int width, int height)
{
  scr_width = width;
  scr_height = height;
  glViewport(0, 0, width, height);
  framebufferResized = true;
}

void processInput(GLFWwindow *window)
{
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);
  camera.processMovement(window);
  glm::vec3 pos = camera.getPos();
  std::cout << pos.x << ", " << pos.y << ", " << pos.z << std::endl;
}

void cursorPosCallback(GLFWwindow *window, double xposin, double yposin)
{
  camera.updateView(static_cast<float>(xposin), static_cast<float>(yposin));
}

void scrollCallback(GLFWwindow *window, double xoffset, double yoffset)
{
  camera.updateZoom(static_cast<float>(yoffset));
}

void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
  ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
  if (ImGui::GetIO().WantCaptureMouse)
    return;

  if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
  {
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    camera_movement = true;
  }
  else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
  {
    camera.firstMouse = true;
    camera_movement = false;
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSetCursorPosCallback(window, nullptr);
  }
}

glm::vec3 *initSumOfSines(int n_waves, float max_amp, float max_freq, float max_speed)
{
  glm::vec3 *waves = new glm::vec3[n_waves];
  for (int i = 0; i < n_waves; i++)
  {
    waves[i].x = max_amp * dis(gen);
    waves[i].y = max_freq * dis(gen);
    waves[i].z = max_speed * dis(gen);
  }
  return waves;
}

float *initRandoms(int n_waves)
{
  float *randoms = new float[n_waves];
  for (int i = 0; i < n_waves; i++)
    randoms[i] = dis(gen);
  return randoms;
}

float *initAngles(int n_waves, float spread_degrees, float wind_dir_degrees)
{
  float *angles = new float[n_waves];
  float wind_dir = glm::radians(wind_dir_degrees);
  float spread = glm::radians(spread_degrees);
  for (int i = 0; i < n_waves; i++)
  {
    wind_dir = glm::radians(dis(gen) * 180.0f);
    angles[i] = wind_dir + (dis(gen) - 0.5f) * 2.0f * spread;
  }
  return angles;
}

// Changed: depth renderbuffer -> depth texture so the screen shader can sample it.
// GL_DEPTH_COMPONENT24 textures are still blittable, so the skybox depth blit is unaffected.
SceneFBO createFBO(unsigned int width, unsigned int height)
{
  SceneFBO fbo;

  glGenFramebuffers(1, &fbo.id);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo.id);

  glGenTextures(1, &fbo.colorTex);
  glBindTexture(GL_TEXTURE_2D, fbo.colorTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo.colorTex, 0);

  glGenTextures(1, &fbo.depthTex);
  glBindTexture(GL_TEXTURE_2D, fbo.depthTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24,
               width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, fbo.depthTex, 0);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    std::cerr << "ERROR::FBO: Framebuffer not complete!" << std::endl;

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  return fbo;
}

void deleteFBO(SceneFBO &fbo)
{
  glDeleteTextures(1, &fbo.depthTex);
  glDeleteTextures(1, &fbo.colorTex);
  glDeleteFramebuffers(1, &fbo.id);
}

unsigned int loadCubemap(const std::string &basePath, const std::vector<std::string> &faces)
{
  unsigned int textureID;
  glGenTextures(1, &textureID);
  glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

  stbi_set_flip_vertically_on_load(false);
  int width, height, nrChannels;
  for (unsigned int i = 0; i < faces.size(); i++)
  {
    std::string path = basePath + "/" + faces[i];
    unsigned char *data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
    if (data)
    {
      GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                   0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
      stbi_image_free(data);
    }
    else
    {
      std::cerr << "ERROR::CUBEMAP: Failed to load face: " << path << std::endl;
      stbi_image_free(data);
    }
  }

  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

  return textureID;
}
