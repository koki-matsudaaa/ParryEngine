#pragma once
#include "pch.h"

struct Transform
{
	XMFLOAT3 position{ 0.0f,0.0f,0.0f };
	XMFLOAT3 rotation{ 0.0f,0.0f,0.0f };
	XMFLOAT3 scale{ 1.0f,1.0f,1.0f };

	// ワールド行列の生成
	XMMATRIX GetWorldMatrix() const
	{
		XMMATRIX s = XMMatrixScaling(scale.x, scale.y, scale.z);
		XMMATRIX r = XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
		XMMATRIX p = XMMatrixTranslation(position.x, position.y, position.z);

		return s * r * p;
	}

	// Yawから前方向のベクトルを取得
	XMFLOAT3 GetForward() const
	{
		return XMFLOAT3(sinf(rotation.y), 0.0f, cosf(rotation.y));
	}

	// Yawから右方向のベクトルを取得 (forwardをY軸まわりに+90度回したもの)
	XMFLOAT3 GetRight() const
	{
		return XMFLOAT3(cosf(rotation.y), 0.0f, -sinf(rotation.y));
	}
};