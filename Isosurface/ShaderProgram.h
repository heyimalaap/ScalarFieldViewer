#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <filesystem>
#include <istream>

namespace fs = std::filesystem; 

class ShaderProgram {
public:
    ShaderProgram() = default;

    static ShaderProgram from_files(
        const fs::path vertex_shader,
        const fs::path fragment_shader
    );

    static ShaderProgram from_files(
        const fs::path vertex_shader,
        const fs::path fragment_shader,
        const fs::path geometry_shader
    );

    static ShaderProgram from_streams(
        std::istream& vertex_shader,
        std::istream& fragment_shader
    );

    static ShaderProgram from_streams(
        std::istream& vertex_shader,
        std::istream& fragment_shader,
        std::istream& geometry_shader
    );

    void use();


    void set(std::string uniform_name, int value);
    void set(std::string uniform_name, float value);
    void set(std::string uniform_name, double value);
    void set(std::string uniform_name, bool value);
    void set(std::string uniform_name, glm::vec2& value);
    void set(std::string uniform_name, glm::vec3& value);
    void set(std::string uniform_name, glm::vec4& value);
    void set(std::string uniform_name, glm::mat4& value);

private:
    ShaderProgram(GLuint id);

    static GLuint compile_and_attach(std::istream& shader, GLenum shader_type, GLuint shader_prog_id);

    GLuint m_id;
};
