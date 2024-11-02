#include "ArcballCamera.h"
#include "MarchingCubes.h"
#include "ShaderProgram.h"
#include "WireframeBoundingBox.h"
#include <GL/gl.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cmath>
#include <glm/common.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <iostream>
#include <memory>
#include <sstream>

#include <VTKParser.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

// Constants
constexpr int WINDOW_WIDTH = 800;
constexpr int WINDOW_HEIGHT = 800;
constexpr char* WINDOW_TITLE = (char* const)"ASSIGNMENT 2";

// Globals
bool mouse_lbtn_pressed = false;
ArcballCamera camera(15.0f);
ShaderProgram wireframe_shader;
ShaderProgram phong_shader;
VTKData data;
float isovalue = 1.0f;

glm::mat4 model = glm::mat4(1.0f);
glm::mat4 view = glm::mat4(1.0f);
glm::mat4 projection = glm::mat4(1.0f);

std::unique_ptr<WireframeBoundingBox> bounding_box;


void calculateFPS(GLFWwindow* window) 
{
    static double last_frame_time = glfwGetTime();
    static size_t frame_count = 0;
    frame_count++;

    double current_frame_time = glfwGetTime();
    double delta = current_frame_time - last_frame_time;

    if (delta >= 1.0) {
        double fps = double(frame_count) / delta;
        std::stringstream ss;
        ss << WINDOW_TITLE << "(FPS: " << fps << ")";
        std::string title = ss.str();
        glfwSetWindowTitle(window, title.c_str());

        frame_count = 0;
        last_frame_time = current_frame_time;
    }
}

void set_projection_matrix(float width, float height)
{
    float L = data.dimension.x * data.spacing.x;
    float H = data.dimension.y * data.spacing.y;
    float W = data.dimension.z * data.spacing.z;
    float diag = std::sqrt(L*L + H*H + W*W) / 2;

    // Calculate the aspect ratio
    float aspectRatio = width / height;

    // Define the orthographic projection clipping planes
    float nearPlane = 100.0f;  // Near clipping plane
    float farPlane = 0.0f;     // Far clipping plane

    // Define the orthographic projection bounds
    float left = -aspectRatio * diag;     // Adjust left based on aspect ratio
    float right = aspectRatio * diag;     // Adjust right based on aspect ratio
    float bottom = -diag;                 // Bottom
    float top = diag;                     // Top

    // Create the orthographic projection matrix
    projection = glm::ortho(left, right, bottom, top, nearPlane, farPlane);
}

void window_resize_callback(GLFWwindow* window, int width, int height)
{
    set_projection_matrix(width, height);
    glViewport(0, 0, width, height);
}

void mouse_move_callback(GLFWwindow* window, double X, double Y)
{
    ImGuiIO& io = ImGui::GetIO();
    io.AddMousePosEvent(X, Y);

    if (!io.WantCaptureMouse)
    {
        if (mouse_lbtn_pressed)
        {
            camera.mouseMove(X, Y);
        }
    }
}

void mouse_click_callback(GLFWwindow* window, int button, int action, int mods)
{
    ImGuiIO& io = ImGui::GetIO();
    io.AddMouseButtonEvent(button, action == GLFW_PRESS);

    if (!io.WantCaptureMouse)
    {
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
            mouse_lbtn_pressed = true;
        } else {
            mouse_lbtn_pressed = false;
            camera.reloadTrigger();
        }
    }
}

void mouse_scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    ImGuiIO& io = ImGui::GetIO();
    io.AddMouseWheelEvent(xoffset, yoffset);

    if (!io.WantCaptureMouse)
    {
        camera.mouseScroll(xoffset);
    }
}

GLuint vert_VBO, normal_VBO, VAO, tricount;

void create_isosurface()
{
    auto [tris, normals] = MarchingCubes::triangulate_field(data.fields[0], isovalue);
    tricount = tris.size();

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, vert_VBO);
    glBufferData(GL_ARRAY_BUFFER, tris.size() * sizeof(glm::vec3), tris.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, normal_VBO);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), normals.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void setup()
{
    data = VTKParser::from_file("data/redseasmall.vtk");


    // upload the vertices to the GPU
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &vert_VBO);
    glGenBuffers(1, &normal_VBO);

    create_isosurface();

    std::cout << "Dimensions: [" 
        << data.dimension.x * data.spacing.x << ", "
        << data.dimension.y * data.spacing.y << ", "
        << data.dimension.z * data.spacing.z << "]\n";

    // Build the bounding box
    float L = (data.dimension.x - 1) * data.spacing.x;
    float H = (data.dimension.y - 1) * data.spacing.y;
    float W = (data.dimension.z - 1) * data.spacing.z;

    bounding_box = std::make_unique<WireframeBoundingBox>(L, H, W);

    wireframe_shader = ShaderProgram::from_files(
            "Isosurface/Shaders/Wireframe.vert",
            "Isosurface/Shaders/Wireframe.frag"
        );

    phong_shader = ShaderProgram::from_files(
            "Isosurface/Shaders/Phong.vert",
            "Isosurface/Shaders/Phong.frag"
        );
    set_projection_matrix(WINDOW_WIDTH, WINDOW_HEIGHT);
}

void draw()
{
    float L = data.dimension.x * data.spacing.x;
    float H = data.dimension.y * data.spacing.y;
    float W = data.dimension.z * data.spacing.z;

    view = camera.getViewMatrix();

    wireframe_shader.use();
    model = glm::mat4(1.0f);
    wireframe_shader.set("model", model);
    wireframe_shader.set("view", view);
    wireframe_shader.set("projection", projection);
    bounding_box->draw();

    // render the isosurface
    auto view_pos = camera.position();
    phong_shader.use();
    model = glm::translate(glm::mat4(1.0f), glm::vec3(-L/2, -H/2, -W/2));
    phong_shader.set("model", model);
    phong_shader.set("view", view);
    phong_shader.set("projection", projection);
    phong_shader.set("viewPos", view_pos);
    glBindVertexArray(VAO);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDrawArrays(GL_TRIANGLES, 0, tricount);
}

int main(int, char**) 
{
    glfwInit();

    glfwWindowHint(GLFW_SCALE_FRAMEBUFFER, GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(
            WINDOW_WIDTH,
            WINDOW_HEIGHT,
            WINDOW_TITLE,
            nullptr,
            nullptr
        );

    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glewInit();
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glEnable(GL_DEPTH_TEST);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init();

    glfwSetFramebufferSizeCallback(window, window_resize_callback);
    glfwSetCursorPosCallback(window, mouse_move_callback);
    glfwSetMouseButtonCallback(window, mouse_click_callback);
    glfwSetScrollCallback(window, mouse_scroll_callback);
    glfwSwapInterval(0);

    setup();
    float L = (data.dimension.x - 1) * data.spacing.x;
    float H = (data.dimension.y - 1) * data.spacing.y;
    float W = (data.dimension.z - 1) * data.spacing.z;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        glClearColor(0.3, 0.3, 0.3, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        {
            ImGui::Begin("Isovalue");
            if(ImGui::SliderFloat("Isovalue", &isovalue, data.fields[0].min_val(), data.fields[0].max_val())) {
                create_isosurface();
            }
            ImGui::End(); 
        }

        draw();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        calculateFPS(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

