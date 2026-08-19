#include "Engine/Graphics/Camera.h"
#include <cmath>

using namespace DirectX;

namespace Engine
{
    void Camera::AddPitch(float radians)
    {
        m_pitch += radians;
        if (m_pitch < minPitch) m_pitch = minPitch;
        if (m_pitch > maxPitch) m_pitch = maxPitch;
    }

    XMFLOAT3 Camera::Eye() const
    {
        // 注視点から、yaw の方向へ distance だけ下がった位置。
        // pitch のぶん持ち上げる。
        const float cp = std::cos(m_pitch);
        return XMFLOAT3(
            m_target.x - std::sin(m_yaw) * cp * m_distance,
            m_target.y + lookHeight + std::sin(m_pitch) * m_distance,
            m_target.z - std::cos(m_yaw) * cp * m_distance);
    }

    XMMATRIX Camera::View() const
    {
        const XMFLOAT3 e = Eye();
        const XMFLOAT3 t(m_target.x, m_target.y + lookHeight, m_target.z);

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