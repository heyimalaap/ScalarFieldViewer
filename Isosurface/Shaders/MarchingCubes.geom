#version 460 core

layout(points) in;
layout(triangle_strip, max_vertices = 12) out;

int edge_verts[12][2] = int[12][2](
    int[2](0, 1),
    int[2](1, 2),
    int[2](2, 3),
    int[2](3, 0),
    int[2](4, 5),
    int[2](5, 6),
    int[2](6, 7),
    int[2](7, 4),
    int[2](0, 4),
    int[2](1, 5),
    int[2](2, 6),
    int[2](3, 7)
);

ivec3 deltas[8] = ivec3[8](
    ivec3(0, 0, 0),
    ivec3(1, 0, 0),
    ivec3(1, 1, 0),
    ivec3(0, 1, 0),
    ivec3(0, 0, 1),
    ivec3(1, 0, 1),
    ivec3(1, 1, 1),
    ivec3(0, 1, 1)
);

in ivec3 geomCubeIndex[];

uniform vec3 dimensions;
uniform vec3 spacing;
uniform float isovalue;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform sampler3D fieldSampler;
uniform sampler3D normalSampler;
uniform isampler2D edgeTableSampler;
uniform isampler2D triTableSampler;

out vec3 fragPos;
out vec3 fragNormal;

float field(ivec3 cubeIndex)
{
    return texelFetch(fieldSampler, cubeIndex, 0).r;
}

vec3 normal(ivec3 cubeIndex)
{
    return texelFetch(normalSampler, cubeIndex, 0).xyz;
}

int edgeTable(int cubeIndex)
{
    return texelFetch(edgeTableSampler, ivec2(cubeIndex, 0), 0).r;
}

int triTable(int cubeIndex, int i)
{
    return texelFetch(triTableSampler, ivec2(i, cubeIndex), 0).r;
}

void main()
{
    vec3 cubeOrigin = vec3(geomCubeIndex[0]) * spacing;
    double scalar_vals[8];
    scalar_vals[0] = field(geomCubeIndex[0] + ivec3(0, 0, 0));
    scalar_vals[1] = field(geomCubeIndex[0] + ivec3(1, 0, 0));
    scalar_vals[2] = field(geomCubeIndex[0] + ivec3(1, 1, 0));
    scalar_vals[3] = field(geomCubeIndex[0] + ivec3(0, 1, 0));
    scalar_vals[4] = field(geomCubeIndex[0] + ivec3(0, 0, 1));
    scalar_vals[5] = field(geomCubeIndex[0] + ivec3(1, 0, 1));
    scalar_vals[6] = field(geomCubeIndex[0] + ivec3(1, 1, 1));
    scalar_vals[7] = field(geomCubeIndex[0] + ivec3(0, 1, 1));

    int cubeIndex = 0;
    if (scalar_vals[0] < isovalue) cubeIndex |= 1;
    if (scalar_vals[1] < isovalue) cubeIndex |= 2;
    if (scalar_vals[2] < isovalue) cubeIndex |= 4;
    if (scalar_vals[3] < isovalue) cubeIndex |= 8;
    if (scalar_vals[4] < isovalue) cubeIndex |= 16;
    if (scalar_vals[5] < isovalue) cubeIndex |= 32;
    if (scalar_vals[6] < isovalue) cubeIndex |= 64;
    if (scalar_vals[7] < isovalue) cubeIndex |= 128;

    if (edgeTable(cubeIndex) == 0)
        return;

    vec3 vertexBuffer[12];
    vec3 normalBuffer[12];
    for (int i = 0; i < 12; i++)
    {
        if ((edgeTable(cubeIndex) & (1 << i)) != 0)
        {
            int v0 = edge_verts[i][0];
            int v1 = edge_verts[i][1];
            vec3 p0 = cubeOrigin + vec3(deltas[v0]) * spacing;
            vec3 p1 = cubeOrigin + vec3(deltas[v1]) * spacing;
            double t = (isovalue - scalar_vals[v0]) / (scalar_vals[v1] - scalar_vals[v0]);
            vertexBuffer[i] = mix(p0, p1, float(t));

            vec3 n0 = normal(geomCubeIndex[0] + ivec3(deltas[v0]));
            vec3 n1 = normal(geomCubeIndex[0] + ivec3(deltas[v1]));
            normalBuffer[i] = normalize(mix(n0, n1, float(t)));
        }
    }

    for (int i = 0; triTable(cubeIndex, i) != 16; i += 3)
    {
        for (int j = 0; j < 3; j++)
        {
            int edge = triTable(cubeIndex, i + j);
            vec3 aPos = vertexBuffer[edge];
            vec3 aNormal = normalBuffer[edge];
            fragPos = vec3(model * vec4(aPos, 1.0f));
            fragNormal = mat3(transpose(inverse(model))) * aNormal;
            gl_Position = projection * view * vec4(fragPos, 1.0f);
            EmitVertex();
        }
        EndPrimitive();
    }


    // Emit a single vertex at each point with the scalar value as geometry color
    // vec3 cubeOrigin = vec3(geomCubeIndex[0]) * spacing;
    // vec3 aPos = cubeOrigin;
    // fragPos = vec3(model * vec4(aPos, 1.0f));
    // gl_Position = projection * view * vec4(fragPos, 1.0f);
    // geomColor = vec3(field(geomCubeIndex[0]));
    // EmitVertex();
    // EndPrimitive();
}

