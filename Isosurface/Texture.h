#pragma once

#include <GL/gl.h>
#include <glm/glm.hpp>
#include <VTKParser.h>

class Texture1D {
public:
    Texture1D();
    Texture1D(GLuint id, int L);

    static Texture1D from_colormap(glm::vec3 low_color, glm::vec3 high_color);
    void bind();
private:
    GLuint m_id;
    int m_L;
};

class Texture3D {
public:
    Texture3D();
    Texture3D(GLuint id, int L, int W, int D);

    static Texture3D from_data(VTKField<double> data, size_t width, size_t height, size_t depth, GLint filter = GL_LINEAR);
    static Texture3D from_intarray(int* data, size_t width, size_t height, size_t depth);
    void bind();
    GLuint id() const;
    Dimension dimension() const;

private:
    GLuint m_id;
    int m_L, m_W, m_D;
};

