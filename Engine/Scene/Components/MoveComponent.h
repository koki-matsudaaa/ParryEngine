#pragma once
#include "pch.h"
#include "GameObject.h"
#include "RigidBody.h"

struct MoveInput
{
	float forward = 0.0f;
	float strafe = 0.0f;
	float turn = 0.0f;
};

class MoveComponent : public GameComponent
{
private:
	MoveInput m_input;
	RigidBody* m_body = nullptr;
	MoveInput m_lastInput;
	float m_camYaw = 0.0f;   // カメラの水平角 (移動の基準)

protected:
	// 載った瞬間に、同じ GameObject 上の RigidBody を探してキャッシュ。
	void OnAttach() override
	{
		m_body = Owner()->GetComponent<RigidBody>();
	}

public:
	// 調整パラメータ (機体の性格を決める数値)
	float accel = 90.0f;
	float turnSpeed = 2.5f;

	// カメラのyawを外から渡す (移動をカメラ基準にする)
	void SetCameraYaw(float yaw) { m_camYaw = yaw; }

	// 毎フレーム、入力を外から渡す (入力読み取りは別の層の仕事)
	void SetInput(const MoveInput& input)
	{
		m_lastInput = input;
		m_input = input;
	}

	bool Update(float dt) override
	{
		if (!m_body) m_body = Owner()->GetComponent<RigidBody>();
		if (!m_body) return true; // 物理ボディが無ければ何もしない

		Transform& tf = Owner()->transform;

		// ── カメラの向き(m_camYaw)を基準に、移動方向を作る ──
		//   スティック前 = カメラの奥へ / スティック右 = カメラの右へ。
		float sinY = sinf(m_camYaw);
		float cosY = cosf(m_camYaw);

		// カメラから見た「前」方向 (ワールドXZ)
		float fwdX = sinY;
		float fwdZ = cosY;
		// カメラから見た「右」方向 (ワールドXZ)
		float rightX = cosY;
		float rightZ = -sinY;

		// 入力を合成した、ワールドでの移動方向。
		float moveX = fwdX * m_input.forward + rightX * m_input.strafe;
		float moveZ = fwdZ * m_input.forward + rightZ * m_input.strafe;

		// ── 加速: 移動方向へ力を加える ──
		XMFLOAT3 force
		{
			moveX * accel,
			0.0f, // 通常移動は水平のみ。縦は重力/ブーストの担当。
			moveZ * accel,
		};
		m_body->AddForce(force);

		// ── 進行方向へ機体を向ける ──
		//   入力があるときだけ、移動方向へなめらかに向き直る。
		float inputMag = sqrtf(m_input.forward * m_input.forward
			+ m_input.strafe * m_input.strafe);
		if (inputMag > 0.1f)
		{
			// 移動方向から、目標の向き(yaw)を求める。
			float targetYaw = atan2f(moveX, moveZ);

			// 現在の向きから目標へ、最短経路でなめらかに回す。
			float currentYaw = tf.rotation.y;
			float diff = targetYaw - currentYaw;
			while (diff > XM_PI)  diff -= XM_2PI;
			while (diff < -XM_PI) diff += XM_2PI;

			float t = 1.0f - expf(-turnSpeed * 2.0f * dt);
			tf.rotation.y = currentYaw + diff * t;
		}

		return true;
	}

	// 最後に受け取った入力 (アニメ方向判定用)
	MoveInput GetLastInput() const
	{
		return m_lastInput;
	}
};