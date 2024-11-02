#include "MarchingCubesGPU.h"
#include "MarchingCubesLUT.h"

void MarchingCubesGPU::generate_LUT_textures()
{
    // Generate edge table texture
    glGenTextures(1, &m_texid_edge_table);
    glBindTexture(GL_TEXTURE_1D, m_texid_edge_table);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_R32I, 256, 0, GL_RED, GL_INT, EDGE_TBL);
    glBindTexture(GL_TEXTURE_1D, 0);

    // Generate triangle table texture
    glGenTextures(1, &m_texid_tri_table);
    glBindTexture(GL_TEXTURE_2D, m_texid_tri_table);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, 16, 256, 0, GL_RED, GL_INT, TRI_TBL);
    glBindTexture(GL_TEXTURE_2D, 0);
}
