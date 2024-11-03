#include "ArcballCamera.h"
#include "ShaderProgram.h"
#include "Texture.h"
#include "WireframeBoundingBox.h"
#include <GL/gl.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cmath>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_common.hpp>
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

enum class RenderMode {
    CPU = 0,
    GPU = 1
};

class SlicingPlane;
class SlicingPlaneGPU;

// Constants
constexpr int WINDOW_WIDTH = 800;
constexpr int WINDOW_HEIGHT = 800;
constexpr char* WINDOW_TITLE = (char* const)"ASSIGNMENT 2";

// Globals
bool mouse_lbtn_pressed = false;
ArcballCamera camera(15.0f);
ShaderProgram wireframe_shader;
ShaderProgram sliceplane_shader;
ShaderProgram sliceplanetex_shader;
Texture1D color_map;
Texture3D data_tex;
VTKData data;
enum class SlicePlaneType {
    XY = 0, YZ = 1, XZ = 2
} slice_plane = SlicePlaneType::XY;
RenderMode render_mode = RenderMode::CPU;
int selected_field = 0;
float plane_ratio = 0.5f;

glm::mat4 model = glm::mat4(1.0f);
glm::mat4 view = glm::mat4(1.0f);
glm::mat4 projection = glm::mat4(1.0f);

std::unique_ptr<WireframeBoundingBox> bounding_box;
std::unique_ptr<SlicingPlane> slicing_plane;
std::unique_ptr<SlicingPlaneGPU> slicing_plane_2;

// Util functions
float lerp(float x, float y, float t) {
  float val = x * (1.f - t) + y * t;
  return val;
}

class SlicingPlane
{
public:
    SlicingPlane(SlicePlaneType type, float L, float H, float W)
        :m_type(type)
    {
        std::vector<float> plane_verts;
        std::vector<GLuint> plane_indicies;

        float topX, topY, topZ;
        float X, Y, Z;
        X = Y = Z = 0;

        switch (m_type) {
            case SlicePlaneType::XY:
                m_sliding_length = W;
                m_sliding_spacing = data.spacing.z;
                m_sliding_dimension = data.dimension.z;
                topX = -L/2;
                topY = -H/2;
                topZ = 0.0f;

                for (int j = 0; j < data.dimension.y; j++)
                {
                    X = 0;
                    for (int i = 0; i < data.dimension.x; i++) {
                        plane_verts.push_back(topX + X);
                        plane_verts.push_back(topY + Y);
                        plane_verts.push_back(topZ + Z);
                        X += data.spacing.x;
                    }
                    Y += data.spacing.y;
                }

                for (int i = 0; i < data.dimension.x - 1; i++)
                {
                    for (int j = 0; j < data.dimension.y - 1; j++) {
                        plane_indicies.push_back(data.dimension.x * j + i);
                        plane_indicies.push_back(data.dimension.x * (j + 1) + i);
                        plane_indicies.push_back(data.dimension.x * (j + 1) + (i + 1));

                        plane_indicies.push_back(data.dimension.x * j + i);
                        plane_indicies.push_back(data.dimension.x * (j + 1) + (i + 1));
                        plane_indicies.push_back(data.dimension.x * j + (i + 1));
                    }
                }
                break;

            case SlicePlaneType::YZ:
                m_sliding_length = L;
                m_sliding_spacing = data.spacing.x;
                m_sliding_dimension = data.dimension.x;
                topX = 0.0f;
                topY = -H/2;
                topZ = -W/2;

                for (int j = 0; j < data.dimension.z; j++)
                {
                    Y = 0;
                    for (int i = 0; i < data.dimension.y; i++) {
                        plane_verts.push_back(topX + X);
                        plane_verts.push_back(topY + Y);
                        plane_verts.push_back(topZ + Z);
                        Y += data.spacing.y;
                    }
                    Z += data.spacing.z;
                }

                for (int i = 0; i < data.dimension.y - 1; i++)
                {
                    for (int j = 0; j < data.dimension.z - 1; j++) {
                        plane_indicies.push_back(data.dimension.y * j + i);
                        plane_indicies.push_back(data.dimension.y * (j + 1) + i);
                        plane_indicies.push_back(data.dimension.y * (j + 1) + (i + 1));

                        plane_indicies.push_back(data.dimension.y * j + i);
                        plane_indicies.push_back(data.dimension.y * (j + 1) + (i + 1));
                        plane_indicies.push_back(data.dimension.y * j + (i + 1));
                    }
                }
                break;

            case SlicePlaneType::XZ:
                m_sliding_length = H;
                m_sliding_spacing = data.spacing.y;
                m_sliding_dimension = data.dimension.y;
                topX = -L/2;
                topY = 0.0f;
                topZ = -W/2;

                for (int j = 0; j < data.dimension.z; j++)
                {
                    X = 0;
                    for (int i = 0; i < data.dimension.x; i++) {
                        plane_verts.push_back(topX + X);
                        plane_verts.push_back(topY + Y);
                        plane_verts.push_back(topZ + Z);
                        X += data.spacing.x;
                    }
                    Z += data.spacing.z;
                }

                for (int i = 0; i < data.dimension.x - 1; i++)
                {
                    for (int j = 0; j < data.dimension.z - 1; j++) {
                        plane_indicies.push_back(data.dimension.x * j + i);
                        plane_indicies.push_back(data.dimension.x * (j + 1) + i);
                        plane_indicies.push_back(data.dimension.x * (j + 1) + (i + 1));

                        plane_indicies.push_back(data.dimension.x * j + i);
                        plane_indicies.push_back(data.dimension.x * (j + 1) + (i + 1));
                        plane_indicies.push_back(data.dimension.x * j + (i + 1));
                    }
                }
                break;
        }

        m_idx_count = plane_indicies.size();

        glGenVertexArrays(1, &m_VAO);
        glGenBuffers(1, &m_vert_VBO);
        glGenBuffers(1, &m_color_VBO);
        glGenBuffers(1, &m_EBO);

        glBindVertexArray(m_VAO);

        glBindBuffer(GL_ARRAY_BUFFER, m_vert_VBO);
        glBufferData(GL_ARRAY_BUFFER, plane_verts.size() * sizeof(float), plane_verts.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, plane_indicies.size() * sizeof(GLuint), plane_indicies.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (GLvoid*)0);
        glEnableVertexAttribArray(0);

        glBindVertexArray(0);

    }

    ~SlicingPlane()
    {
        glDeleteVertexArrays(1, &m_VAO);
        glDeleteBuffers(1, &m_vert_VBO);
        glDeleteBuffers(1, &m_color_VBO);
        glDeleteBuffers(1, &m_EBO);
    }

    void setColorData(VTKField<double>& data)
    {
        float pos = lerp(0, m_sliding_length, m_ratio);
        size_t p1 = pos / m_sliding_spacing;
        size_t p2 = std::min(p1+1, m_sliding_dimension - 1);

        float residue = ((pos / m_sliding_spacing) - p1) / m_sliding_spacing;

        glm::vec3 color1(0.0f, 0.0f, 1.0f);
        glm::vec3 color2(1.0f, 0.0f, 0.0f);

        std::vector<float> color_buffer;

        switch (m_type)
        {
            case SlicePlaneType::XY:
                for (int j = 0; j < data.dimension.y; j++)
                {
                    for (int i = 0; i < data.dimension.x; i++) {
                        float d = lerp(data(i, j, p1),data(i, j, p2), residue);
                        float t = (d - data.min_val()) / (data.max_val() - data.min_val());
                        auto c = glm::vec3(lerp(color1.r, color2.r, t), lerp(color1.g, color2.g, t), lerp(color1.b, color2.b, t));
                        color_buffer.push_back(c.r);
                        color_buffer.push_back(c.g);
                        color_buffer.push_back(c.b);
                    }
                }
                break;

            case SlicePlaneType::XZ:
                for (int j = 0; j < data.dimension.z; j++)
                {
                    for (int i = 0; i < data.dimension.x; i++) {
                        float d = lerp(data(i, p1, j),data(i, p2, j), residue);
                        float t = (d - data.min_val()) / (data.max_val() - data.min_val());
                        auto c = glm::vec3(lerp(color1.r, color2.r, t), lerp(color1.g, color2.g, t), lerp(color1.b, color2.b, t));
                        color_buffer.push_back(c.r);
                        color_buffer.push_back(c.g);
                        color_buffer.push_back(c.b);
                    }
                }
                break;

            case SlicePlaneType::YZ:
                for (int j = 0; j < data.dimension.z; j++)
                {
                    for (int i = 0; i < data.dimension.y; i++) {
                        float d = lerp(data(p1, i, j),data(p2, i, j), residue);
                        float t = (d - data.min_val()) / (data.max_val() - data.min_val());
                        auto c = glm::vec3(lerp(color1.r, color2.r, t), lerp(color1.g, color2.g, t), lerp(color1.b, color2.b, t));
                        color_buffer.push_back(c.r);
                        color_buffer.push_back(c.g);
                        color_buffer.push_back(c.b);
                    }
                }
                break;
        }

        glBindVertexArray(m_VAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_color_VBO);
        glBufferData(GL_ARRAY_BUFFER, color_buffer.size() * sizeof(float), color_buffer.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (GLvoid*)0);
        glEnableVertexAttribArray(1);
        glBindVertexArray(0);
    }

    void draw()
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glBindVertexArray(m_VAO);
        glDrawElements(GL_TRIANGLES, m_idx_count, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    glm::mat4 getModelMatrix()
    {
        glm::mat4 model = glm::mat4(1.0f);
        switch (m_type) {
            case SlicePlaneType::XY:
                model = glm::translate(model, glm::vec3(0.0f, 0.0f, lerp(-m_sliding_length/2, m_sliding_length/2, m_ratio)));
                break;
            case SlicePlaneType::YZ:
                model = glm::translate(model, glm::vec3(lerp(-m_sliding_length/2, m_sliding_length/2, m_ratio), 0.0f, 0.0f));
                break;
            case SlicePlaneType::XZ:
                model = glm::translate(model, glm::vec3(0.0f, lerp(-m_sliding_length/2, m_sliding_length/2, m_ratio), 0.0f));
                break;
        }
        return model;
    }
    
    SlicePlaneType type()
    {
        return m_type;
    }

    float m_ratio = 0.5f; // Between 0.0f to 1.0f
private:
    float m_sliding_length, m_sliding_spacing;
    size_t m_sliding_dimension;
    SlicePlaneType m_type;    

    GLuint m_VAO, m_vert_VBO, m_color_VBO, m_EBO;
    size_t m_idx_count;
};

class SlicingPlaneGPU
{
public:
    SlicingPlaneGPU(SlicePlaneType type, float L, float H, float W)
        :m_type(type)
    {
        std::vector<float> plane_verts;
        std::vector<float> plane_st = {
            0.0f, 0.0f,
            0.0f, 1.0f,
            1.0f, 0.0f,
            1.0f, 1.0f
        };
        std::vector<GLuint> plane_indicies;

        switch (m_type) {
            case SlicePlaneType::XY:
                m_sliding_length = W;
                m_sliding_spacing = data.spacing.z;
                m_sliding_dimension = data.dimension.z;

                plane_verts.push_back(-L/2);
                plane_verts.push_back(-H/2);
                plane_verts.push_back(0.0f);

                plane_verts.push_back(-L/2);
                plane_verts.push_back(H/2);
                plane_verts.push_back(0.0f);

                plane_verts.push_back(L/2);
                plane_verts.push_back(-H/2);
                plane_verts.push_back(0.0f);

                plane_verts.push_back(L/2);
                plane_verts.push_back(H/2);
                plane_verts.push_back(0.0f);

                break;

            case SlicePlaneType::YZ:
                m_sliding_length = L;
                m_sliding_spacing = data.spacing.x;
                m_sliding_dimension = data.dimension.x;

                plane_verts.push_back(0.0f);
                plane_verts.push_back(-H/2);
                plane_verts.push_back(-W/2);

                plane_verts.push_back(0.0f);
                plane_verts.push_back(H/2);
                plane_verts.push_back(-W/2);

                plane_verts.push_back(0.0f);
                plane_verts.push_back(-H/2);
                plane_verts.push_back(W/2);

                plane_verts.push_back(0.0f);
                plane_verts.push_back(H/2);
                plane_verts.push_back(W/2);

                break;

            case SlicePlaneType::XZ:
                m_sliding_length = H;
                m_sliding_spacing = data.spacing.y;
                m_sliding_dimension = data.dimension.y;

                plane_verts.push_back(-L/2);
                plane_verts.push_back(0.0f);
                plane_verts.push_back(-W/2);

                plane_verts.push_back(L/2);
                plane_verts.push_back(0.0f);
                plane_verts.push_back(-W/2);

                plane_verts.push_back(-L/2);
                plane_verts.push_back(0.0f);
                plane_verts.push_back(W/2);

                plane_verts.push_back(L/2);
                plane_verts.push_back(0.0f);
                plane_verts.push_back(W/2);

                break;
        }

        plane_indicies.push_back(0);
        plane_indicies.push_back(2);
        plane_indicies.push_back(1);

        plane_indicies.push_back(2);
        plane_indicies.push_back(1);
        plane_indicies.push_back(3);

        glGenVertexArrays(1, &m_VAO);
        glGenBuffers(1, &m_vert_VBO);
        glGenBuffers(1, &m_st_VBO);
        glGenBuffers(1, &m_EBO);

        glBindVertexArray(m_VAO);

        glBindBuffer(GL_ARRAY_BUFFER, m_vert_VBO);
        glBufferData(GL_ARRAY_BUFFER, plane_verts.size() * sizeof(float), plane_verts.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (GLvoid*)0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, m_st_VBO);
        glBufferData(GL_ARRAY_BUFFER, plane_st.size() * sizeof(float), plane_st.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (GLvoid*)0);
        glEnableVertexAttribArray(1);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, plane_indicies.size() * sizeof(GLuint), plane_indicies.data(), GL_STATIC_DRAW);

        glBindVertexArray(0);
    }

    void draw()
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glBindVertexArray(m_VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    glm::mat4 getModelMatrix()
    {
        glm::mat4 model = glm::mat4(1.0f);
        switch (m_type) {
            case SlicePlaneType::XY:
                model = glm::translate(model, glm::vec3(0.0f, 0.0f, lerp(-m_sliding_length/2, m_sliding_length/2, m_ratio)));
                break;
            case SlicePlaneType::YZ:
                model = glm::translate(model, glm::vec3(lerp(-m_sliding_length/2, m_sliding_length/2, m_ratio), 0.0f, 0.0f));
                break;
            case SlicePlaneType::XZ:
                model = glm::translate(model, glm::vec3(0.0f, lerp(-m_sliding_length/2, m_sliding_length/2, m_ratio), 0.0f));
                break;
        }
        return model;
    }
    
    SlicePlaneType type()
    {
        return m_type;
    }

    float m_ratio = 0.5f; // Between 0.0f to 1.0f
private:
    float m_sliding_length, m_sliding_spacing;
    size_t m_sliding_dimension;
    SlicePlaneType m_type;

    GLuint m_VAO, m_vert_VBO, m_st_VBO, m_EBO, m_texID;
};

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

void create_stuff() {
    slicing_plane->setColorData(data.fields[selected_field]);
    color_map = Texture1D::from_colormap(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(1.0f,0.0f, 0.0f));
    data_tex = Texture3D::from_data(
            data.fields[selected_field],
            data.dimension.x,
            data.dimension.y,
            data.dimension.z
        );
}

void setup()
{
    data = VTKParser::from_file("data/redseasmall.vtk");
    std::cout << "Dimensions: [" 
        << data.dimension.x * data.spacing.x << ", "
        << data.dimension.y * data.spacing.y << ", "
        << data.dimension.z * data.spacing.z << "]\n";

    // Build the bounding box
    float L = (data.dimension.x - 1) * data.spacing.x;
    float H = (data.dimension.y - 1) * data.spacing.y;
    float W = (data.dimension.z - 1) * data.spacing.z;

    bounding_box = std::make_unique<WireframeBoundingBox>(L, H, W);

    slicing_plane = std::make_unique<SlicingPlane>(SlicePlaneType::XY, L, H, W);
    slicing_plane->setColorData(data.fields[selected_field]);

    slicing_plane_2 = std::make_unique<SlicingPlaneGPU>(SlicePlaneType::XY, L, H, W);

    wireframe_shader = ShaderProgram::from_files(
            "Slicer/Shaders/Wireframe.vert",
            "Slicer/Shaders/Wireframe.frag"
        );
    sliceplane_shader = ShaderProgram::from_files(
            "Slicer/Shaders/SlicingPlane.vert",
            "Slicer/Shaders/SlicingPlane.frag"
        );
    sliceplanetex_shader = ShaderProgram::from_files(
            "Slicer/Shaders/SlicingPlaneTex.vert",
            "Slicer/Shaders/SlicingPlaneTex.frag"
        );

    color_map = Texture1D::from_colormap(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(1.0f,0.0f, 0.0f));

    data_tex = Texture3D::from_data(
            data.fields[selected_field],
            data.dimension.x,
            data.dimension.y,
            data.dimension.z
        );

    set_projection_matrix(WINDOW_WIDTH, WINDOW_HEIGHT);
}

void draw()
{
    view = camera.getViewMatrix();

    wireframe_shader.use();
    model = glm::mat4(1.0f);
    wireframe_shader.set("model", model);
    wireframe_shader.set("view", view);
    wireframe_shader.set("projection", projection);
    bounding_box->draw();

    if (render_mode == RenderMode::CPU) {
        model = slicing_plane->getModelMatrix();
        sliceplane_shader.use();
        sliceplane_shader.set("model", model);
        sliceplane_shader.set("view", view);
        sliceplane_shader.set("projection", projection);
        slicing_plane->draw();
    } else {
        sliceplanetex_shader.use();
        color_map.bind();
        data_tex.bind();
        model = slicing_plane_2->getModelMatrix();
        sliceplanetex_shader.set("model", model);
        sliceplanetex_shader.set("view", view);
        sliceplanetex_shader.set("projection", projection);
        sliceplanetex_shader.set("t", slicing_plane_2->m_ratio);
        sliceplanetex_shader.set("data_min", data.fields[selected_field].min_val());
        sliceplanetex_shader.set("data_max", data.fields[selected_field].max_val());
        sliceplanetex_shader.set("colourmapTexture", 0);
        sliceplanetex_shader.set("dataTexture", 1);
        sliceplanetex_shader.set("planeType", (int)slicing_plane_2->type());
        slicing_plane_2->draw();
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
                    slicing_plane->m_ratio = plane_ratio;
                    create_stuff();
                }
                if (ImGui::MenuItem("GPU", nullptr, render_mode == RenderMode::GPU)) {
                    render_mode = RenderMode::GPU;
                    slicing_plane_2->m_ratio = plane_ratio;
                    create_stuff();
                }
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("Fields")) {
                for (int i = 0; i < field_names.size(); i++) {
                    if (ImGui::MenuItem(field_names[i].c_str(), nullptr, selected_field == i)) {
                        selected_field = i;
                        create_stuff();
                    }
                }
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }
        {
            ImGui::Begin("Slicing");

            if(ImGui::RadioButton("XY", slicing_plane->type() == SlicePlaneType::XY)) {
                slicing_plane = std::make_unique<SlicingPlane>(SlicePlaneType::XY, L, H, W);
                slicing_plane->setColorData(data.fields[selected_field]);

                slicing_plane_2 = std::make_unique<SlicingPlaneGPU>(SlicePlaneType::XY, L, H, W);

                slicing_plane->m_ratio = plane_ratio;
                slicing_plane_2->m_ratio = plane_ratio;
            }
            ImGui::SameLine();

            if(ImGui::RadioButton("YZ", slicing_plane->type() == SlicePlaneType::YZ)) {
                slicing_plane = std::make_unique<SlicingPlane>(SlicePlaneType::YZ, L, H, W);
                slicing_plane->setColorData(data.fields[selected_field]);

                slicing_plane_2 = std::make_unique<SlicingPlaneGPU>(SlicePlaneType::YZ, L, H, W);

                slicing_plane->m_ratio = plane_ratio;
                slicing_plane_2->m_ratio = plane_ratio;
            }
            ImGui::SameLine();

            if(ImGui::RadioButton("XZ", slicing_plane->type() == SlicePlaneType::XZ)) {
                slicing_plane = std::make_unique<SlicingPlane>(SlicePlaneType::XZ, L, H, W);
                slicing_plane->setColorData(data.fields[selected_field]);

                slicing_plane_2 = std::make_unique<SlicingPlaneGPU>(SlicePlaneType::XZ, L, H, W);

                slicing_plane->m_ratio = plane_ratio;
                slicing_plane_2->m_ratio = plane_ratio;
            }

            if(ImGui::SliderFloat("t", &plane_ratio, 0.0f, 1.0f)) {
                if (render_mode == RenderMode::CPU) {
                    slicing_plane->m_ratio = plane_ratio;
                    slicing_plane->setColorData(data.fields[selected_field]);
                } else {
                    slicing_plane_2->m_ratio = plane_ratio;
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

