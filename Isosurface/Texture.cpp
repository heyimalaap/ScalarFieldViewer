#include "Texture.h"

#include <vector>

Texture1D::Texture1D() {}

Texture1D::Texture1D(GLuint id, int L)
    : m_id(id), m_L(L) {}

Texture1D Texture1D::from_colormap(glm::vec3 low_color, glm::vec3 high_color)
{
    std::vector<float> colormap;
    for (int i = 0; i < 256; i++)
    {
        float t = i / 255.0f;
        glm::vec3 c = glm::mix(low_color, high_color, t);
        colormap.push_back(c.r);
        colormap.push_back(c.g);
        colormap.push_back(c.b);
    }
    
    GLuint id;
    glGenTextures(1, &id);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_1D, id);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, 256, 0, GL_RGB, GL_FLOAT, colormap.data());
    glBindTexture(GL_TEXTURE_1D, 0);

    return Texture1D(id, 256);
}

void Texture1D::bind()
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_1D, m_id);
}

Texture3D::Texture3D() {}

Texture3D::Texture3D(GLuint id, int L, int W, int D)
    : m_id(id), m_L(L), m_W(W), m_D(D) {}

Texture3D Texture3D::from_data(VTKField<double> data, size_t width, size_t height, size_t depth)
{
    std::vector<float> fdata;
    for (size_t z = 0; z < depth; z++)
    {
        for (size_t y = 0; y < height; y++)
        {
            for (size_t x = 0; x < width; x++)
            {
                fdata.push_back(data(x, y, z));
            }
        }
    }
    GLuint id;
    glGenTextures(1, &id);
    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_3D, id);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage3D(GL_TEXTURE_3D, 0, GL_R32F, width, height, depth, 0, GL_RED, GL_FLOAT, fdata.data());

    glBindTexture(GL_TEXTURE_3D, 0);

    return Texture3D(id, width, height, depth);
}

void Texture3D::bind()
{
    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_3D, m_id);
}

GLuint Texture3D::id() const 
{ 
    return m_id; 
}

Dimension Texture3D::dimension() const 
{ 
    return Dimension { m_L, m_W, m_D };
}
