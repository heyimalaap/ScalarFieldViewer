#pragma once

#include <glm/glm.hpp>

class ArcballCamera
{
public:
    ArcballCamera(float radius);
    glm::mat4 getViewMatrix();
    void mouseMove(float X, float Y);
    void mouseScroll(float deltaS);
    void reloadTrigger();
    glm::vec3 position();

private:
    void recomputePosition();

private:
    glm::vec3 m_position;
    glm::vec3 m_target;

    float m_radius;
    float m_theta;
    float m_phi;

    bool m_trigger = true;

    float m_move_sensitivity = 0.1f;
    float m_zoom_sensitivity = 1.0f;
    float m_min_radius = 1.0f;
    float m_max_radius = 100.0f;
};
