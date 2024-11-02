#include "ArcballCamera.h"

#include "glm/ext/matrix_transform.hpp"
#include <glm/glm.hpp>

ArcballCamera::ArcballCamera(float radius)
    :m_radius(radius), m_target(glm::vec3(0.0f, 0.0f, 0.0f)), m_theta(0.0f), m_phi(0.0f)
{
    recomputePosition();
}

glm::mat4 ArcballCamera::getViewMatrix()
{
    return glm::lookAt(m_position, m_target, glm::vec3(0.0f, 1.0f, 0.0f));
} 

void ArcballCamera::mouseMove(float X, float Y)
{
    static float oldX;
    static float oldY;

    if (m_trigger)
    {
        m_trigger = false;
        oldX = X;
        oldY = Y;
    }

    float deltaX = X - oldX;
    float deltaY = oldY - Y;

    m_theta += deltaX * m_move_sensitivity;
    m_phi += deltaY * m_move_sensitivity;

    m_phi = glm::clamp(m_phi, -90.0f, 90.0f);

    oldX = X;
    oldY = Y;

    recomputePosition();
}


void ArcballCamera::mouseScroll(float deltaS)
{
    m_radius -= deltaS * m_zoom_sensitivity;
    m_radius = glm::clamp(m_radius, m_min_radius, m_max_radius);

    recomputePosition();
}

void ArcballCamera::reloadTrigger()
{
    m_trigger = true;
}

glm::vec3 ArcballCamera::position()
{
    return m_position;
}

void ArcballCamera::recomputePosition()
{
    m_position.x = m_radius * cos(glm::radians(m_phi)) * sin(glm::radians(m_theta));
    m_position.y = m_radius * sin(glm::radians(m_phi));
    m_position.z = m_radius * cos(glm::radians(m_phi)) * cos(glm::radians(m_theta));
}
