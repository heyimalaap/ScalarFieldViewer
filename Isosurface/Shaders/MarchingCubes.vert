#version 460 core

layout(location = 0) in vec3 aCubeIndex;

out ivec3 geomCubeIndex;

void main()
{
    geomCubeIndex = ivec3(aCubeIndex.x, aCubeIndex.y, aCubeIndex.z);
}
