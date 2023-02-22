#include "Camera.h"

#include "glm/gtc/matrix_transform.hpp"

void Camera::Setup(glm::vec2 ViewportSize)
{
    SetViewportSize(ViewportSize);
    UpdateVectors();
    RecalculateViewMatrix();
    RecalculateProjectionMatrix();
}

void Camera::ProcessMovement(CameraMoveDirection Direction, float Speed, float ElapsedTime)
{
    switch (Direction)
    {
    case CameraMoveDirection::Forward:
        m_Position += m_Forward * Speed * ElapsedTime;
        break;

    case CameraMoveDirection::Backward:
        m_Position -= m_Forward * Speed * ElapsedTime;
        break;

    case CameraMoveDirection::Left:
        m_Position -= m_Right * Speed * ElapsedTime;
        break;

    case CameraMoveDirection::Right:
        m_Position += m_Right * Speed * ElapsedTime;
        break;

    case CameraMoveDirection::Up:
        m_Position += m_Up * Speed * ElapsedTime;
        break;

    case CameraMoveDirection::Down:
        m_Position -= m_Up * Speed * ElapsedTime;
        break;

    default:
        break;
    }

    RecalculateViewMatrix();
}

void Camera::ProcessRotation(glm::vec2 CursorDelta, float Sensitivity, float ElapsedTime)
{
    static bool bFirstTime = true;
    if (bFirstTime)
    {
        bFirstTime = false;
        return;
    }

    float XOffset = CursorDelta.x;
    float YOffset = CursorDelta.y;

    XOffset *= Sensitivity * ElapsedTime;
    YOffset *= Sensitivity * ElapsedTime;

    m_RotYawPitch.x += XOffset;
    m_RotYawPitch.y += YOffset;

    if (m_RotYawPitch.y > 89.0f)
    {
        m_RotYawPitch.y = 89.0f;
    }
    if (m_RotYawPitch.y < -89.0f)
    {
        m_RotYawPitch.y = -89.0f;
    }

    UpdateVectors();
}

void Camera::SetViewportSize(glm::vec2 ViewportSize)
{
    m_ViewportSize = ViewportSize;
    RecalculateViewMatrix();
}

void Camera::SetNearClip(float NearClip)
{
    m_NearClip = NearClip;
    RecalculateProjectionMatrix();
}

void Camera::SetFarClip(float FarClip)
{
    m_FarClip = FarClip;
    RecalculateProjectionMatrix();
}

void Camera::UpdateVectors()
{
    glm::vec2 const RotRadians = glm::radians(m_RotYawPitch);

    float const Yaw   = RotRadians.x;
    float const Pitch = RotRadians.y;

    m_Forward.x = std::cos(Pitch) * std::cos(Yaw);
    m_Forward.y = std::sin(Pitch);
    m_Forward.z = std::cos(Pitch) * std::sin(Yaw);
    m_Forward   = glm::normalize(m_Forward);

    m_Right = glm::normalize(glm::cross(m_Forward, glm::vec3{0.0f, 1.0f, 0.0f}));
    m_Up    = glm::normalize(glm::cross(m_Right, m_Forward));

    RecalculateViewMatrix();
}

void Camera::RecalculateViewMatrix()
{
    m_View = glm::lookAt(m_Position, m_Position + m_Forward, m_Up);
}

void Camera::RecalculateProjectionMatrix()
{
    m_Projection = glm::perspective(
        glm::radians(m_FoVDegrees), m_ViewportSize.x / m_ViewportSize.y, m_NearClip, m_FarClip
    );
}
