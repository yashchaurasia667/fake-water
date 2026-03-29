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

#include <learnOpengl/shader.h>
#include <learnOpengl/camera.h>
#include <learnOpengl/utils.h>
#include <learnOpengl/vertexArray.h>
#include <learnOpengl/vertexBuffer.h>
#include <learnOpengl/vertexBufferLayout.h>
#include <learnOpengl/indexBuffer.h>

unsigned int scr_width = 1280, scr_height = 720;
Camera camera(glm::vec3(0.0f, 0.0f, 10.0f), 45.0f, 0.1f, 15.0f);
bool camera_movement = false;

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution dis(0.0f, 1.0f);

void framebufferSizeCallback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow *window);
void cursorPosCallback(GLFWwindow *window, double xposin, double yposin);
void scrollCallback(GLFWwindow *window, double xoffset, double yoffset);
void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods);
void GLFWMouseButtonCallback(GLFWwindow *window, int button, int action, int mods);

glm::vec3 *initSumOfSines(int n_waves, float max_amp, float max_freq, float max_speed);
float *initFloats(int n_waves);

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
  // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  // glfwSetCursorPosCallback(window, cursorPosCallback);
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
    Shader shader("../shaders/vertex.vs", "../shaders/fragment.fs");

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

    // imgui controlled wave params
    int num_waves = 32;
    float max_speed = 2.5f;
    float *speed = initFloats(num_waves);
    float *angles = initFloats(num_waves);
    float *amp_coeff = initFloats(1);
    float *amp = initFloats(1), *freq = initFloats(1);

    // imgui controlled light params
    float ambient;
    glm::vec3 light_dir = glm::vec3(0.5f, 0.7f, 0.8f);
    glm::vec3 light_color = glm::vec3(1.0f);
    glm::vec3 water_color = glm::vec3(0.1, 0.4, 0.5);

    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glClearColor(0.4, 0.4, 0.4, 0.4);
    while (!glfwWindowShouldClose(window))
    {
      camera.updateFrame();

      glfwPollEvents();
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      processInput(window);

      // wave params
      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();
      {
        ImGui::Begin("Wave params");

        ImGui::SliderInt("num waves", &num_waves, 16, 160);
        ImGui::SliderFloat("amp", amp, 0.0f, 10.0f, "%.2f");
        ImGui::SliderFloat("freq", freq, 0.0f, 10.0f, "%.2f");
        ImGui::SliderFloat("max_speed", &max_speed, -10.0f, 10.0f, "%.2f");

        if (ImGui::Button("Init wave"))
        {
          std::cout << "Re-initializing wave" << std::endl;
          speed = initFloats(num_waves);
          angles = initFloats(num_waves);
        }
        ImGui::End();
      }

      {
        ImGui::Begin("Light params");

        ImGui::SliderFloat("ambient", &ambient, 0.0f, 1.0f, "%.2f");

        if (ImGui::CollapsingHeader("light direction"))
        {
          ImGui::SliderFloat("x", &light_dir.x, 0.0f, 1.0f, "%.2f");
          ImGui::SliderFloat("y", &light_dir.y, 0.0f, 1.0f, "%.2f");
          ImGui::SliderFloat("z", &light_dir.z, 0.0f, 1.0f, "%.2f");
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

      glm::mat4 model = glm::mat4(1.0f);
      glm::mat4 view = camera.getViewMatrix();
      glm::mat4 projection = glm::perspective(camera.getFov(), (float)scr_width / (float)scr_height, 0.1f, 1000.0f);
      model = glm::translate(model, glm::vec3(-50.0f, -5.0f, -150.0f));
      // model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

      shader.bind();
      shader.setMat4("model", model);
      shader.setMat4("view", view);
      shader.setMat4("projection", projection);

      // vertex uniforms
      shader.setInt("u_numWaves", num_waves);
      shader.setFloat("u_amp", *amp);
      shader.setFloat("u_freq", *freq);
      shader.setFloat("u_amp_coeff", *amp_coeff);
      shader.setFloat("u_freq_coeff", 2.0f - *amp_coeff);
      for (int i = 0; i < num_waves; i++)
      {
        shader.setFloat(("u_speed[" + std::to_string(i) + "]").c_str(), speed[i] * max_speed);
        shader.setFloat(("u_angle[" + std::to_string(i) + "]").c_str(), angles[i]);
      }
      shader.setFloat("u_time", (float)glfwGetTime());

      // fragment uniforms
      shader.setFloat("u_ambient", ambient);
      shader.setVec3("u_lightDir", light_dir);
      shader.setVec3("u_lightColor", light_color);
      shader.setVec3("u_waterColor", water_color);
      shader.setVec3("u_viewPos", camera.getPos());
      shader.setInt("u_shininess", 128);

      vao.bind();
      ebo.bind();
      glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_INT, 0);

      ImGui::Render();
      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

      glfwSwapBuffers(window);
    }
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
}

void processInput(GLFWwindow *window)
{
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);
  camera.processMovement(window);
}

void cursorPosCallback(GLFWwindow *window, double xposin, double yposin)
{
  float xpos = static_cast<float>(xposin);
  float ypos = static_cast<float>(yposin);
  camera.updateView(xpos, ypos);
}

void scrollCallback(GLFWwindow *window, double xoffset, double yoffset)
{
  float yoff = static_cast<float>(yoffset);
  camera.updateZoom(yoff);
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

float *initFloats(int n_waves)
{
  float *angles = new float[n_waves];

  float wind_angle = 3.14159f / 4.0f;

  for (int i = 0; i < n_waves; i++)
  {
    float spread = (dis(gen) - 0.5f) * 1.0f;
    angles[i] = wind_angle + spread;
  }
  return angles;
}

void GLFWMouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
  ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
  // if (Renderer::glfw_mouse_button_callback && !ImGui::GetIO().WantCaptureMouse)
  //   Renderer::glfw_mouse_button_callback(window, button, action, mods);
}
