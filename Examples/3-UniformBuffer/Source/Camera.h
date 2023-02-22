#ifndef VULAKNLEARNING_CAMERA
#define VULAKNLEARNING_CAMERA

#include <glm/glm.hpp>

enum class CameraMoveDirection
{
    Forward,
    Backward,
    Left,
    Right,
    Up,
    Down
};

class Camera
{
public:
    Camera() = default;

    void Setup(glm::vec2 ViewportSize);

    void ProcessMovement(CameraMoveDirection Direction, float Speed, float ElapsedTime);
    void ProcessRotation(glm::vec2 CursorDelta, float Sensitivity, float ElapsedTime);

    glm::vec3 GetPosition() const { return m_Position; }
    void      SetPosition(glm::vec3 const &Position) { m_Position = Position; }

    glm::vec3 GetForward() const { return m_Forward; }
    glm::vec3 GetUp() const { return m_Up; }
    glm::vec3 GetRight() const { return m_Right; }

    float GetYawDegrees() const { return m_RotYawPitch.x; }
    float GetPitchDegrees() const { return m_RotYawPitch.y; }

    void SetViewportSize(glm::vec2 ViewportSize);

    void SetNearClip(float NearClip);
    void SetFarClip(float FarClip);

    glm::mat4 GetViewMatrix() const { return m_View; }
    glm::mat4 GetProjectionMatrix() const { return m_Projection; }

private:
    void UpdateVectors();

    void RecalculateViewMatrix();
    void RecalculateProjectionMatrix();

private:
    glm::vec3 m_Position = glm::vec3{0.0f, 0.0f, 0.0f};

    glm::vec3 m_Forward = glm::vec3{0.0f, 0.0f, -1.0f};
    glm::vec3 m_Up      = glm::vec3{0.0f, 1.0f, 0.0f};
    glm::vec3 m_Right   = glm::vec3{1.0f, 0.0f, 0.0f};

    glm::vec2 m_RotYawPitch = glm::vec2{-90.0f, 0.0f}; // degrees

    glm::vec2 m_ViewportSize = glm::vec2{0.0f, 0.0f};

    float m_FoVDegrees = 60.0f;

    float m_NearClip = 0.01f;
    float m_FarClip  = 100.0f;

    glm::mat4 m_View       = glm::mat4(1.0f);
    glm::mat4 m_Projection = glm::mat4(1.0f);
};

#endif