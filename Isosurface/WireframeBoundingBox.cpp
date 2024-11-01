#include "WireframeBoundingBox.h"

#include <array>

WireframeBoundingBox::WireframeBoundingBox(float L, float H, float W)
{
    std::array<float, 24> br_verts = {
        -L/2, -H/2, -W/2,
        -L/2, -H/2,  W/2,
        -L/2,  H/2, -W/2,
        -L/2,  H/2,  W/2,
         L/2, -H/2, -W/2,
         L/2, -H/2,  W/2,
         L/2,  H/2, -W/2,
         L/2,  H/2,  W/2
    };

    std::array<GLuint, 4*6> br_indices = {
        0, 1, 3, 2,
        5, 4, 6, 7,
        3, 7, 6, 2,
        0, 4, 5, 1,
        4, 0, 2, 6,
        1, 5, 7, 3
    };

    glGenVertexArrays(6, m_VAO);
    glGenBuffers(1, &m_VBO);
    glGenBuffers(6, m_EBO);

    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, br_verts.size() * sizeof(float), br_verts.data(), GL_STATIC_DRAW);

    for (int i = 0; i < 6; i++) 
    {
        glBindVertexArray(m_VAO[i]);

        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (GLvoid*)0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO[i]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, 4 * sizeof(GLuint), br_indices.data() + 4*i, GL_STATIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
}

WireframeBoundingBox::~WireframeBoundingBox()
{
    glDeleteVertexArrays(6, m_VAO);
    glDeleteBuffers(1, &m_VBO);
    glDeleteBuffers(6, m_EBO);
}

void WireframeBoundingBox::set_shader(ShaderProgram& sp)
{
    m_shader = sp;
}

void WireframeBoundingBox::draw()
{
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    for (int i = 0; i < 6; i++)
    {
        glBindVertexArray(m_VAO[i]);
        glDrawElements(GL_LINE_LOOP, 4, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
}
