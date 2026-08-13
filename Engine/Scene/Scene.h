#pragma once
#include "pch.h"
#include "GameObject.h"
#include "RenderComponent.h"
#include "ColliderComponent.h"
#include "HealthComponent.h"
#include <functional>

class Scene
{
private:
	std::vector<std::unique_ptr<GameObject>> m_object;

	// 敵を撃破したときに呼ぶ処理。スコア加算などを外から差し込む。
	// Scene にスコアの知識を持たせないための逃がし口。
	std::function<void(GameObject*)> m_onEnemyDestroyed;

	// オブジェクトを削除する直前に呼ぶ。生ポインタを持っている側に外してもらう。
	std::function<void(GameObject*)> m_onObjectDestroyed;

	// 弾と敵の球同士の当たり判定。当たったら弾は消え、敵はダメージを受ける。
	void CheckBulletHits()
	{
		// 弾と敵をそれぞれ集める。
		std::vector<GameObject*> bullets;
		std::vector<GameObject*> enemies;
		for (auto& obj : m_object)
		{
			if (!obj->IsAlive()) continue;
			if (obj->GetTag() == ObjectTag::Bullet) bullets.push_back(obj.get());
			else if (obj->GetTag() == ObjectTag::Enemy) enemies.push_back(obj.get());
		}

		// 総当たり。
		for (GameObject* b : bullets)
		{
			ColliderComponent* bc = b->GetComponent<ColliderComponent>();
			if (!bc) continue;

			for (GameObject* e : enemies)
			{
				if (!e->IsAlive()) continue; // 同フレームで既に倒された敵は飛ばす
				ColliderComponent* ec = e->GetComponent<ColliderComponent>();
				if (!ec) continue;

				// 中心間の距離の2乗と、半径の和の2乗を比べる (sqrt を避ける)。
				XMFLOAT3 bp = bc->GetCenter();
				XMFLOAT3 ep = ec->GetCenter();
				float dx = bp.x - ep.x;
				float dy = bp.y - ep.y;
				float dz = bp.z - ep.z;
				float distSq = dx * dx + dy * dy + dz * dz;

				float radSum = bc->GetRadius() + ec->GetRadius();
				float radSumSq = radSum * radSum;

				if (distSq < radSumSq)
				{
					// 命中: 敵にダメージ、弾は消滅。
					if (HealthComponent* hp = e->GetComponent<HealthComponent>())
					{
						hp->TakeDamage(34.0f); // 仮: 3発で 100 を削り切る

						// TakeDamage が HP0 で Destroy() を呼ぶので、
						// 生存フラグの変化で「この一撃で倒した」と分かる。
						if (!e->IsAlive() && m_onEnemyDestroyed)
						{
							m_onEnemyDestroyed(e);
						}
					}
					b->Destroy();
					break; // この弾は使い切ったので次の弾へ
				}
			}
		}
	}

public:
	// 敵の撃破時に呼ばれる処理を差し込む。main 側でスコア加算をつなぐ。
	void SetOnEnemyDestroyed(std::function<void(GameObject*)> fn)
	{
		m_onEnemyDestroyed = std::move(fn);
	}

	// オブジェクトが削除される直前に呼ばれる処理を差し込む。
	void SetOnObjectDestroyed(std::function<void(GameObject*)> fn)
	{
		m_onObjectDestroyed = std::move(fn);
	}

	// 空の GameObject を生成してシーンに登録し、ポインタを返す。
	// 呼び出し側はこのポインタに AddComponent() していく。
	GameObject* CreateObject()
	{
		auto obj = std::make_unique<GameObject>();
		GameObject* raw = obj.get();
		m_object.push_back(std::move(obj));
		return raw;
	}

	// 毎フレーム呼ばれる。有効な GameObject をすべて更新する。
	void Update(float dt) 
	{
		// (1) 全オブジェクト更新 (移動・弾の前進など)
		for (auto& obj : m_object)
		{
			if (obj->IsEnabled())
			{
				obj->Update(dt);
			}
		}

		// (2) 当たり判定: 弾 × 敵 の総当たり。
		CheckBulletHits();

		// (3) 死んだオブジェクトを取り除く。
		RemoveDead();
	}

	// 死んだ (Destroy された) オブジェクトを実際に取り除く。
	// 消す直前に「このオブジェクトはもう消える」と外へ知らせる。
	// ロックオンやスポナーが持っている生ポインタを外してもらうため。
	// 通常は Update の最後に呼ばれるが、リセット直後など Update を回さずに
	// 掃除したい場面があるので public にしてある。
	void RemoveDead()
	{
		if (m_onObjectDestroyed)
		{
			for (auto& obj : m_object)
			{
				if (!obj->IsAlive()) m_onObjectDestroyed(obj.get());
			}
		}

		m_object.erase(
			std::remove_if(m_object.begin(), m_object.end(),
				[](const std::unique_ptr<GameObject>& o) { return !o->IsAlive(); }),
			m_object.end());
	}

	// ── 描画フェーズ用 ──────────────────────────
	// 描画すべき (GameObject, RenderComponent) のペアを集めて返す。
	// Renderer はこれを受け取り、各 model の Draw を行列付きで呼ぶ。
	// 毎フレーム集め直すので、非表示や非アクティブな物は自然に外れる。
	struct Renderable
	{
		GameObject* object;   // ワールド行列を取るため
		RenderComponent* render;   // モデルと可視状態を取るため
	};

	std::vector<Renderable> CollectRenderables() const
	{
		std::vector<Renderable> list;
		for (auto& obj : m_object)
		{
			if (!obj->IsEnabled()) continue;

			RenderComponent* rc = obj->GetComponent<RenderComponent>();
			if (rc && rc->IsVisible() && rc->GetModel())
			{
				list.push_back({ obj.get(), rc });
			}
		}
		return list;
	}

	// 描画などで全オブジェクトを走査したいとき用。
	const std::vector<std::unique_ptr<GameObject>>& GetObjects() const
	{
		return m_object;
	}

	// Clean確認
	size_t GetObjectCount() const
	{
		return m_object.size();
	}

	// 指定タグのオブジェクトをまとめて消す。リトライ時に残弾を掃除するのに使う。
	void DestroyByTag(ObjectTag tag)
	{
		for (auto& obj : m_object)
		{
			if (obj->GetTag() == tag) obj->Destroy();
		}
	}
};

