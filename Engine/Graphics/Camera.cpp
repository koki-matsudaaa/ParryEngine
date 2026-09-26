#include "Engine/Graphics/Camera.h"
#include <cmath>

using namespace DirectX;

namespace Engine
{
    void Camera::SetPitch(float radians)
    {
        m_pitch = radians;
        if (m_pitch < minPitch) m_pitch = minPitch;
        if (m_pitch > maxPitch) m_pitch = maxPitch;
    }

    void Camera::AddPitch(float radians)
    {
        SetPitch(m_pitch + radians);
    }

    XMFLOAT3 Camera::LookPoint() const
    {
        // 右へ sideOffset だけずらす
        const float rx = std::cos(m_yaw);
        const float rz = -std::sin(m_yaw);

        return XMFLOAT3(
            m_target.x + rx * sideOffset,
            m_target.y + lookHeight,
            m_target.z + rz * sideOffset);
    }

    XMFLOAT3 Camera::Eye() const
    {
        // 見ている点から、yaw の方向へ distance だけ下がった位置。
        const XMFLOAT3 look = LookPoint();
        const float cp = std::cos(m_pitch);

        return XMFLOAT3(
            look.x - std::sin(m_yaw) * cp * m_distance,
            look.y + std::sin(m_pitch) * m_distance,
            look.z - std::cos(m_yaw) * cp * m_distance);
    }

    XMMATRIX Camera::View() const
    {
        const XMFLOAT3 e = Eye();
        const XMFLOAT3 t = LookPoint();

        return XMMatrixLookAtLH(
            XMLoadFloat3(&e),
            XMLoadFloat3(&t),
            XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
    }

    XMMATRIX Camera::Projection() const
    {
        return XMMatrixPerspectiveFovLH(fovY, aspect, nearZ, farZ);
    }
}