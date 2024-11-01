cmake_minimum_required(VERSION 3.30)

project(Assignment2)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(COPY data DESTINATION ${CMAKE_BINARY_DIR})

add_library(
    VTKParser
    VTKParser/VTKParser.h
    VTKParser/VTKParser.cpp
)

add_executable(
    VTKParserTest
    VTKParserTest/main.cpp
)
target_link_libraries(VTKParserTest PUBLIC VTKParser)
target_include_directories(
    VTKParserTest PUBLIC
    "${PROJECT_SOURCE_DIR}/VTKParser"
)

find_package(glfw3 CONFIG REQUIRED)
find_package(GLEW REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

add_executable(
    Slicer
    Slicer/main.cpp
    Slicer/ArcballCamera.cpp
    Slicer/ShaderProgram.cpp
    Slicer/Texture.cpp
    Slicer/WireframeBoundingBox.cpp
)
target_link_libraries(Slicer PRIVATE glfw GLEW::GLEW glm::glm-header-only imgui::imgui)
target_link_libraries(Slicer PUBLIC VTKParser)
target_include_directories(
    Slicer PUBLIC
    "${PROJECT_SOURCE_DIR}/VTKParser"
)

add_executable(
    Isosurface
    Isosurface/main.cpp
    Isosurface/ArcballCamera.cpp
    Isosurface/ShaderProgram.cpp
    Isosurface/Texture.cpp
    Isosurface/WireframeBoundingBox.cpp
)
target_link_libraries(Isosurface PRIVATE glfw GLEW::GLEW glm::glm-header-only imgui::imgui)
target_link_libraries(Isosurface PUBLIC VTKParser)
target_include_directories(
    Isosurface PUBLIC
    "${PROJECT_SOURCE_DIR}/VTKParser"
)
