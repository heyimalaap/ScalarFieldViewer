#version 460 core

#define PLANE_XY 0
#define PLANE_YZ 1
#define PLANE_XZ 2

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTex;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float t;
uniform int planeType;

out vec3 vertexColor;
out vec3 texCoord;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0f);
    vertexColor = vec3(1.0f, 1.0f, 1.0f);

    if (planeType == PLANE_XY)
        texCoord = vec3(aTex, t);
    else if (planeType == PLANE_YZ)
        texCoord = vec3(t, aTex.y, aTex.x);
    else if (planeType == PLANE_XZ)
        texCoord = vec3(aTex.y, t, aTex.x);
}
