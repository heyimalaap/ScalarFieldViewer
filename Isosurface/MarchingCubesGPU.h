#pragma once

#include "ShaderProgram.h"
#include <GL/gl.h>

class MarchingCubesGPU
{
    void generate_LUT_textures();

    GLuint m_texid_edge_table;
    GLuint m_texid_tri_table;
    ShaderProgram m_shader;
};
