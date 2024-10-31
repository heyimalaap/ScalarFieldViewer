#include "ShaderProgram.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <glm/gtc/type_ptr.hpp>

ShaderProgram ShaderProgram::from_files(
    const fs::path vertex_shader,
    const fs::path fragment_shader
) {
    std::ifstream vs_file(vertex_shader, std::ios::in);
    std::ifstream fs_file(fragment_shader, std::ios::in);

    return from_streams(vs_file, fs_file);
}

ShaderProgram ShaderProgram::from_files(
    const fs::path vertex_shader,
    const fs::path fragment_shader,
    const fs::path geometry_shader
) {
    std::ifstream vs_file(vertex_shader, std::ios::in);
    std::ifstream fs_file(fragment_shader, std::ios::in);
    std::ifstream gs_file(geometry_shader, std::ios::in);

    return from_streams(vs_file, fs_file, gs_file);
}

GLuint ShaderProgram::compile_and_attach(std::istream& shader, GLenum shader_type, GLuint shader_prog_id) {
    std::stringstream ss;
    ss << shader.rdbuf();
    std::string src(ss.str());
    const char* c_src = src.c_str();

    GLuint id = glCreateShader(shader_type);
    glShaderSource(id, 1, &c_src, NULL);
    glCompileShader(id);
    int success;
    char infoLog[512];
    glGetShaderiv(id, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(id, 512, NULL, infoLog);
        std::cerr << "Failed to compile shader: " << infoLog << "\n";
    }
    glAttachShader(shader_prog_id, id);
    return id;
}

ShaderProgram ShaderProgram::from_streams(
    std::istream& vertex_shader,
    std::istream& fragment_shader
) {
    GLuint shader_id = glCreateProgram();
    GLuint vs_id, fs_id;

    vs_id = ShaderProgram::compile_and_attach(vertex_shader, GL_VERTEX_SHADER, shader_id);
    fs_id = ShaderProgram::compile_and_attach(fragment_shader, GL_FRAGMENT_SHADER, shader_id);

    char infoLog[512];
    int success;
    glLinkProgram(shader_id);
    glGetProgramiv(shader_id, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shader_id, 512, NULL, infoLog);
        std::cout << "Shader linking failed: " << infoLog << std::endl;
    }

    glDeleteShader(vs_id);
    glDeleteShader(fs_id);
    return ShaderProgram(shader_id);
}

ShaderProgram ShaderProgram::from_streams(
    std::istream& vertex_shader,
    std::istream& fragment_shader,
    std::istream& geometry_shader
) {
    GLuint shader_id = glCreateProgram();
    GLuint vs_id, fs_id, gs_id;

    vs_id = ShaderProgram::compile_and_attach(vertex_shader, GL_VERTEX_SHADER, shader_id);
    fs_id = ShaderProgram::compile_and_attach(fragment_shader, GL_FRAGMENT_SHADER, shader_id);
    gs_id = ShaderProgram::compile_and_attach(geometry_shader, GL_GEOMETRY_SHADER, shader_id);


    char infoLog[512];
    int success;
    glLinkProgram(shader_id);
    glGetProgramiv(shader_id, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shader_id, 512, NULL, infoLog);
        std::cout << "Shader linking failed: " << infoLog << std::endl;
    }

    glDeleteShader(vs_id);
    glDeleteShader(fs_id);
    glDeleteShader(gs_id);

    return ShaderProgram(shader_id);
}

ShaderProgram::ShaderProgram(GLuint id)
    :m_id(id) {}

void ShaderProgram::use() {
    glUseProgram(m_id);
}

void ShaderProgram::set(std::string uniform_name, int value) {
    glUniform1i(glGetUniformLocation(m_id, uniform_name.c_str()), value);
}

void ShaderProgram::set(std::string uniform_name, float value) {
    glUniform1f(glGetUniformLocation(m_id, uniform_name.c_str()), value);
}

void ShaderProgram::set(std::string uniform_name, double value) {
    glUniform1f(glGetUniformLocation(m_id, uniform_name.c_str()), value);
}

void ShaderProgram::set(std::string uniform_name, bool value) {
    glUniform1f(glGetUniformLocation(
        m_id,
        uniform_name.c_str()),
        static_cast<bool>(value)
    );
}

void ShaderProgram::set(std::string uniform_name, glm::vec2& value) {
    glUniform2fv(
        glGetUniformLocation(m_id, uniform_name.c_str()),
        1,
        glm::value_ptr(value)
    );
}

void ShaderProgram::set(std::string uniform_name, glm::vec3& value) {
    glUniform3fv(
        glGetUniformLocation(m_id, uniform_name.c_str()),
        1,
        glm::value_ptr(value)
    );
}

void ShaderProgram::set(std::string uniform_name, glm::vec4& value) {
    glUniform4fv(
        glGetUniformLocation(m_id, uniform_name.c_str()),
        1,
        glm::value_ptr(value)
    );
}

void ShaderProgram::set(std::string uniform_name, glm::mat4& value) {
    glUniformMatrix4fv(
        glGetUniformLocation(m_id, uniform_name.c_str()),
        1,
        GL_FALSE,
        glm::value_ptr(value)
    );
}
