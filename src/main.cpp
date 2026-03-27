#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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
Camera camera(glm::vec3(0.0f, 0.0f, 10.0f), 45.0f, 0.1f, 9.5f);
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution dis(0.0f, 1.0f);

void framebufferSizeCallback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow *window);
void cursorPosCallback(GLFWwindow *window, double xposin, double yposin);
void scrollCallback(GLFWwindow *window, double xoffset, double yoffset);
glm::vec3 *initSumOfSines(int n_waves, float max_amp, float max_freq, float max_speed);
float *initAngles(int n_waves);

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
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glfwSetCursorPosCallback(window, cursorPosCallback);
  glfwSetScrollCallback(window, scrollCallback);
  glEnable(GL_DEPTH_TEST);

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

    int num_waves = 6;
    glm::vec3 *waves = initSumOfSines(num_waves, 0.8f, 1.2f, 2.0f);
    float *angles = initAngles(num_waves);

    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glClearColor(0.4, 0.4, 0.4, 0.4);
    while (!glfwWindowShouldClose(window))
    {
      camera.updateFrame();

      glfwPollEvents();
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      processInput(window);

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
      for (int i = 0; i < num_waves; i++)
      {
        shader.setFloat(("u_amp[" + std::to_string(i) + "]").c_str(), waves[i].x);
        shader.setFloat(("u_freq[" + std::to_string(i) + "]").c_str(), waves[i].y);
        shader.setFloat(("u_speed[" + std::to_string(i) + "]").c_str(), waves[i].z);
        shader.setFloat(("u_angle[" + std::to_string(i) + "]").c_str(), angles[i]);
      }
      shader.setFloat("u_time", (float)glfwGetTime());
      
      // fragment uniforms
      shader.setVec3("u_lightDir", glm::vec3(-0.4f, 0.7f, -0.8f));
      shader.setVec3("u_lightColor", glm::vec3(1.0f));
      shader.setVec3("u_waterColor", glm::vec3(0.1f, 0.4f, 0.6f));
      shader.setVec3("u_viewPos", camera.getPos());
      shader.setInt("u_shininess", 512);

      vao.bind();
      ebo.bind();
      glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_INT, 0);

      glfwSwapBuffers(window);
    }
  }

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

float *initAngles(int n_waves)
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