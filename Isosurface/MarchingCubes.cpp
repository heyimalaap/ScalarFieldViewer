#include "MarchingCubes.h"
#include "MarchingCubesLUT.h"

static constexpr int x_delta[8] = {0, 1, 1, 0, 0, 1, 1, 0};
static constexpr int y_delta[8] = {0, 0, 1, 1, 0, 0, 1, 1};
static constexpr int z_delta[8] = {0, 0, 0, 0, 1, 1, 1, 1};


std::pair<std::vector<glm::vec3>,std::vector<glm::vec3>> MarchingCubes::triangulate_field(VTKField<double>& field, double isovalue)
{
    auto gradient = compute_gradient(field);
    std::vector<glm::vec3> triangle_vertices;
    std::vector<glm::vec3> vertex_normals;

    glm::ivec3 field_dim = glm::ivec3(field.dimension.x, field.dimension.y, field.dimension.z);
    for (int x = 0; x < field_dim.x - 1; x++) {
        for (int y = 0; y < field_dim.y - 1; y++) {
            for (int z = 0; z < field_dim.z - 1; z++) {
                triangulate_cell(triangle_vertices, vertex_normals, field, gradient, glm::ivec3(x, y, z), isovalue);
            }
        }
    }

    return { triangle_vertices, vertex_normals };
}

std::vector<std::vector<std::vector<glm::vec3>>> MarchingCubes::compute_gradient(VTKField<double>& field)
{
    std::vector<std::vector<std::vector<glm::vec3>>> gradients;

    gradients.resize(field.dimension.x);
    for(int i = 0; i < field.dimension.x; i++) {
        gradients[i].resize(field.dimension.y);
        for(int j = 0; j < field.dimension.y; j++) {
            gradients[i][j].resize(field.dimension.z);
            for(int k = 0; k < field.dimension.z; k++) {
                glm::vec3 gradient = glm::vec3(0.0f);
                if (i > 0 && i < field.dimension.x - 1) {
                    gradient.x = (field(i + 1, j, k) - field(i - 1, j, k)) / (2 * field.spacing.x);
                }
                if (j > 0 && j < field.dimension.y - 1) {
                    gradient.y = (field(i, j + 1, k) - field(i, j - 1, k)) / (2 * field.spacing.y);
                }
                if (k > 0 && k < field.dimension.z - 1) {
                    gradient.z = (field(i, j, k + 1) - field(i, j, k - 1)) / (2 * field.spacing.z);
                }
                gradients[i][j][k] = gradient;
            }
        }
    }

    // Boundary conditions: Forward and backward differences
    for (int i = 0; i < field.dimension.x; i++) {
        for (int j = 0; j < field.dimension.y; j++) {
            gradients[i][j][0].z = (field(i, j, 1) - field(i, j, 0)) / field.spacing.z;
            gradients[i][j][field.dimension.z - 1].z = (field(i, j, field.dimension.z - 1) - field(i, j, field.dimension.z - 2)) / field.spacing.z;
        }
    }

    for (int i = 0; i < field.dimension.x; i++) {
        for (int k = 0; k < field.dimension.z; k++) {
            gradients[i][0][k].y = (field(i, 1, k) - field(i, 0, k)) / field.spacing.y;
            gradients[i][field.dimension.y - 1][k].y = (field(i, field.dimension.y - 1, k) - field(i, field.dimension.y - 2, k)) / field.spacing.y;
        }
    }

    for (int j = 0; j < field.dimension.y; j++) {
        for (int k = 0; k < field.dimension.z; k++) {
            gradients[0][j][k].x = (field(1, j, k) - field(0, j, k)) / field.spacing.x;
            gradients[field.dimension.x - 1][j][k].x = (field(field.dimension.x - 1, j, k) - field(field.dimension.x - 2, j, k)) / field.spacing.x;
        }
    }

    return gradients;
}

void MarchingCubes::triangulate_cell(std::vector<glm::vec3>& triangle_vertices, std::vector<glm::vec3>& vertex_normals,VTKField<double>& field, std::vector<std::vector<std::vector<glm::vec3>>>& gradients, glm::ivec3 cell_origin, double isovalue)
{
    double scalar_vals[8] = { 
        field(cell_origin.x, cell_origin.y, cell_origin.z),
        field(cell_origin.x + 1, cell_origin.y, cell_origin.z),
        field(cell_origin.x + 1, cell_origin.y + 1, cell_origin.z),
        field(cell_origin.x, cell_origin.y + 1, cell_origin.z),
        field(cell_origin.x, cell_origin.y, cell_origin.z + 1),
        field(cell_origin.x + 1, cell_origin.y, cell_origin.z + 1),
        field(cell_origin.x + 1, cell_origin.y + 1, cell_origin.z + 1),
        field(cell_origin.x, cell_origin.y + 1, cell_origin.z + 1)
    };

    std::vector<glm::vec3> grads = {
        gradients[cell_origin.x][cell_origin.y][cell_origin.z],
        gradients[cell_origin.x + 1][cell_origin.y][cell_origin.z],
        gradients[cell_origin.x + 1][cell_origin.y + 1][cell_origin.z],
        gradients[cell_origin.x][cell_origin.y + 1][cell_origin.z],
        gradients[cell_origin.x][cell_origin.y][cell_origin.z + 1],
        gradients[cell_origin.x + 1][cell_origin.y][cell_origin.z + 1],
        gradients[cell_origin.x + 1][cell_origin.y + 1][cell_origin.z + 1],
        gradients[cell_origin.x][cell_origin.y + 1][cell_origin.z + 1]
    };

    int cube_index = 0;
    if (scalar_vals[0] < isovalue) cube_index |= 1;
    if (scalar_vals[1] < isovalue) cube_index |= 2;
    if (scalar_vals[2] < isovalue) cube_index |= 4;
    if (scalar_vals[3] < isovalue) cube_index |= 8;
    if (scalar_vals[4] < isovalue) cube_index |= 16;
    if (scalar_vals[5] < isovalue) cube_index |= 32;
    if (scalar_vals[6] < isovalue) cube_index |= 64;
    if (scalar_vals[7] < isovalue) cube_index |= 128;

    if(EDGE_TBL[cube_index] == 0) {
        return;
    }

    glm::vec3 vertices[12];
    glm::vec3 normals[12];
    for (int i = 0; i < 12; i++) {
        if (EDGE_TBL[cube_index] & (1 << i)) {
            int v1 = EDGE_VERT_IDX[i].first;
            int v2 = EDGE_VERT_IDX[i].second;
            
            glm::vec3 p1 = glm::vec3((cell_origin.x + x_delta[v1]) * field.spacing.x, (cell_origin.y + y_delta[v1]) * field.spacing.y, (cell_origin.z + z_delta[v1]) * field.spacing.z);
            glm::vec3 p2 = glm::vec3((cell_origin.x + x_delta[v2]) * field.spacing.x, (cell_origin.y + y_delta[v2]) * field.spacing.y, (cell_origin.z + z_delta[v2]) * field.spacing.z);
            double t = (isovalue - scalar_vals[v1]) / (scalar_vals[v2] - scalar_vals[v1]);
            vertices[i] = glm::mix(p1, p2, t);

            // Trilinear interpolation for the normals
            glm::vec3 g1 = grads[v1];
            glm::vec3 g2 = grads[v2];
            normals[i] = glm::normalize(glm::mix(g1, g2, t));
        }
    }

    for (int i = 0; TRI_TBL[cube_index][i] != -1; i += 3) {
        triangle_vertices.push_back(vertices[TRI_TBL[cube_index][i]]);
        vertex_normals.push_back(normals[TRI_TBL[cube_index][i]]);
        triangle_vertices.push_back(vertices[TRI_TBL[cube_index][i + 1]]);
        vertex_normals.push_back(normals[TRI_TBL[cube_index][i + 1]]);
        triangle_vertices.push_back(vertices[TRI_TBL[cube_index][i + 2]]);
        vertex_normals.push_back(normals[TRI_TBL[cube_index][i + 2]]);
    }
}
