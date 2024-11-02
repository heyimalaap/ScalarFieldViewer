#pragma once

#include <glm/glm.hpp>
#include <utility>
#include <vector>
#include <VTKParser.h>

class MarchingCubes
{
public:
    static std::pair<std::vector<glm::vec3>,std::vector<glm::vec3>> triangulate_field(VTKField<double>& field, double isovalue);
    static std::vector<std::vector<std::vector<glm::vec3>>> compute_gradient(VTKField<double>& field);

private:
    static void triangulate_cell(std::vector<glm::vec3>& triangle_vertices, std::vector<glm::vec3>& vertex_normals, VTKField<double>& field, std::vector<std::vector<std::vector<glm::vec3>>>& gradients,glm::ivec3 cell_origin, double isovalue);
};
