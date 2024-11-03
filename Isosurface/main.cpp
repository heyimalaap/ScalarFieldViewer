#include "ArcballCamera.h"
#include "MarchingCubes.h"
#include "MarchingCubesLUT.h"
#include "ShaderProgram.h"
#include "WireframeBoundingBox.h"
#include <GL/gl.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cmath>
#include <glm/common.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/fwd.hpp>
#include <iostream>
#include <memory>
#include <sstream>

#include <VTKParser.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <string>

enum class RenderMode {
    CPU = 0,
    GPU = 1
};

// Constants
constexpr int WINDOW_WIDTH = 800;
constexpr int WINDOW_HEIGHT = 800;
constexpr char* WINDOW_TITLE = (char* const)"ASSIGNMENT 2";

// Globals
bool mouse_lbtn_pressed = false;
ArcballCamera camera(15.0f);
ShaderProgram wireframe_shader;
ShaderProgram phong_shader;
ShaderProgram marching_cube_shader;
VTKData data;
float isovalue = 1.0f;
RenderMode render_mode = RenderMode::CPU;
int selected_field = 0;

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
GLuint gs_VBO, gs_VAO;

void create_isosurface()
{
    auto [tris, normals] = MarchingCubes::triangulate_field(data.fields[selected_field], isovalue);
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

GLuint fieldTextureID;
GLuint normalTextureID;
GLuint edgeTableTextureID;
GLuint triTableTextureID;

void create_gs_textures()
{
    auto gradients = MarchingCubes::compute_gradient(data.fields[selected_field]);
    std::vector<glm::vec3> grad_texture_data;
    for (int k = 0; k < data.dimension.z; k++) {
        for (int j = 0; j < data.dimension.y; j++) {
            for (int i = 0; i < data.dimension.x; i++) {
                grad_texture_data.push_back(gradients[i][j][k]);
            }
        }
    }

    std::vector<float> fieldData;
    for (int k = 0; k < data.dimension.z; k++) {
        for (int j = 0; j < data.dimension.y; j++) {
            for (int i = 0; i < data.dimension.x; i++) {
                fieldData.push_back(data.fields[selected_field](i, j, k));
            }
        }
    }

    glGenTextures(1, &fieldTextureID);
    glBindTexture(GL_TEXTURE_3D, fieldTextureID);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_R32F, data.dimension.x, data.dimension.y, data.dimension.z, 0, GL_RED, GL_FLOAT, fieldData.data());
    glBindTexture(GL_TEXTURE_3D, 0);

    glGenTextures(1, &normalTextureID);
    glBindTexture(GL_TEXTURE_3D, normalTextureID);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB32F, data.dimension.x, data.dimension.y, data.dimension.z, 0, GL_RGB, GL_FLOAT, grad_texture_data.data());
    glBindTexture(GL_TEXTURE_3D, 0);

    // Upload the edge table texture
    glGenTextures(1, &edgeTableTextureID);
    glBindTexture(GL_TEXTURE_2D, edgeTableTextureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I , 256, 1, 0, GL_RED_INTEGER, GL_INT, &EDGE_TBL);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Upload the triangle table texture
    glGenTextures(1, &triTableTextureID);
    glBindTexture(GL_TEXTURE_2D, triTableTextureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, 16, 256, 0, GL_RED_INTEGER, GL_INT, &TRI_TBL);
    glBindTexture(GL_TEXTURE_2D, 0);

}

void create_gs_lattice()
{
    std::vector<glm::vec3> lattice;
    for (int i = 0; i < data.dimension.x - 1; i++) {
        for (int j = 0; j < data.dimension.y - 1; j++) {
            for (int k = 0; k < data.dimension.z - 1; k++) {
                lattice.push_back(glm::vec3(i, j, k));
            }
        }
    }

    glBindVertexArray(gs_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, gs_VBO);
    glBufferData(GL_ARRAY_BUFFER, lattice.size() * sizeof(glm::ivec3), lattice.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void create_stuff_for_current_field() 
{
    if (render_mode == RenderMode::CPU) {
        create_isosurface();
    } else {
        create_gs_textures();
    }
}

void setup()
{
    data = VTKParser::from_file("data/redseasmall.vtk");


    // upload the vertices to the GPU
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &vert_VBO);
    glGenBuffers(1, &normal_VBO);

    create_isosurface();

    glGenVertexArrays(1, &gs_VAO);
    glGenBuffers(1, &gs_VBO);

    create_gs_lattice();
    create_gs_textures();

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

    marching_cube_shader = ShaderProgram::from_files(
            "Isosurface/Shaders/MarchingCubes.vert",
            "Isosurface/Shaders/MarchingCubes.frag",
            "Isosurface/Shaders/MarchingCubes.geom"
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

    if (render_mode == RenderMode::CPU) {
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
    } else {
        auto view_pos = camera.position();
        auto dim_vec = glm::vec3(data.dimension.x, data.dimension.y, data.dimension.z);
        auto spacing_vec = glm::vec3(data.spacing.x, data.spacing.y, data.spacing.z);
        model = glm::mat4(1.0f);
        marching_cube_shader.use();
        model = glm::translate(glm::mat4(1.0f), glm::vec3(-L/2, -H/2, -W/2));
        marching_cube_shader.set("model", model);
        marching_cube_shader.set("view", view);
        marching_cube_shader.set("projection", projection);
        marching_cube_shader.set("viewPos", view_pos);
        marching_cube_shader.set("dimension", dim_vec);
        marching_cube_shader.set("spacing", spacing_vec);
        marching_cube_shader.set("isovalue", isovalue);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_3D, fieldTextureID);
        marching_cube_shader.set("fieldSampler", 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_3D, normalTextureID);
        marching_cube_shader.set("normalSampler", 1);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, edgeTableTextureID);
        marching_cube_shader.set("edgeTableSampler", 2);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, triTableTextureID);
        marching_cube_shader.set("triTableSampler", 3);
        glBindVertexArray(gs_VAO);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDrawArrays(GL_POINTS, 0, data.dimension.x * data.dimension.y * data.dimension.z);
    }
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

    std::vector<std::string> field_names;
    for (auto& field : data.fields) {
        field_names.push_back(field.name);
    }

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        glClearColor(0.3, 0.3, 0.3, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Render Mode")) {
                if (ImGui::MenuItem("CPU", nullptr, render_mode == RenderMode::CPU)) {
                    render_mode = RenderMode::CPU;
                    create_stuff_for_current_field();
                }
                if (ImGui::MenuItem("GPU", nullptr, render_mode == RenderMode::GPU)) {
                    render_mode = RenderMode::GPU;
                    create_stuff_for_current_field();
                }
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("Fields")) {
                for (int i = 0; i < field_names.size(); i++) {
                    if (ImGui::MenuItem(field_names[i].c_str(), nullptr, selected_field == i)) {
                        selected_field = i;
                        create_stuff_for_current_field();
                    }
                }
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }

        {
            ImGui::Begin("Isovalue");
            if(ImGui::SliderFloat("Isovalue", &isovalue, data.fields[selected_field].min_val(), data.fields[selected_field].max_val())) {
                if (render_mode == RenderMode::CPU) {
                    create_isosurface();
                }
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

