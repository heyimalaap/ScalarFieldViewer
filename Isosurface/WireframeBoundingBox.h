#pragma once

#include "ShaderProgram.h"
#include <GL/gl.h>

class WireframeBoundingBox
{
public:
    WireframeBoundingBox(float L, float H, float W);
    ~WireframeBoundingBox();
    void set_shader(ShaderProgram& sp);
    void draw();
private:
    GLuint m_VAO[6], m_VBO, m_EBO[6];
    ShaderProgram m_shader;
};

