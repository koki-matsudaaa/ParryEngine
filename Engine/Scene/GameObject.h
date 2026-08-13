#pragma once
#include "pch.h"
#include "Transform.h"

class GameObject;

class GameComponent
{
private:
	GameObject* m_owner = nullptr;

protected:
	// 継承先が初期化したいときに override する
	virtual void OnAttach() {}

	// 所有者の GameObject を取得する。Transform などにアクセスするため。
	GameObject* Owner() const 
	{ 
		return m_owner;
	}

public:
	virtual ~GameComponent() = default;

	// GameObject に載せられた瞬間に1度だけ呼ばれる。所有者を受け取る
	void Attach(GameObject* owner)
	{
		m_owner = owner;
		OnAttach();
	}

	// 毎フレーム呼ばれる。dt は前フレームからの経過秒。
	// false を返すとこのコンポーネントは寿命を終え、削除される。
	virtual bool Update(float dt) = 0;
};

enum class ObjectTag
{
	None,
	Player,
	Bullet,
	Enemy,
};

class GameObject
{
private:
	std::vector <std::unique_ptr<GameComponent>> m_components;
	bool m_enabled = true;
	bool m_alive = true;
	ObjectTag m_tag = ObjectTag::None;

public:
	Transform transform;

	// コンポーネントを追加する。所有権は GameObject が持つ (unique_ptr)。
	// 追加したコンポーネントのポインタを返すので、呼び出し側で設定に使える。
	template<class T, class... Args>
	T* AddComponent(Args&&... args)
	{
		auto comp = std::make_unique<T>(std::forward<Args>(args)...);
		T* raw = comp.get();
		raw->Attach(this);
		m_components.push_back(std::move(comp));
		return raw;
	}

	// 載っているコンポーネントを型で探す。
	template<class T>
	T* GetComponent() const
	{
		for (auto& c : m_components) 
		{
			if (T* hit = dynamic_cast<T*>(c.get()))
				return hit;
		}
		return nullptr;
	}

	// 毎フレーム呼ばれる。全コンポーネントの Update を順に回す。
	// Update が false を返したコンポーネントはここで除去される。
	void Update(float dt)
	{
		for (auto it = m_components.begin(); it != m_components.end(); )
		{
			bool alive = (*it)->Update(dt);
			if (!alive)
			{
				it = m_components.erase(it);
			}
			else
			{
				++it;
			}
		}
	}

	void SetTag(ObjectTag t)
	{
		m_tag = t;
	}

	ObjectTag GetTag() const
	{
		return m_tag;
	}



	bool IsEnabled() const 
	{ 
		return m_enabled;
	}

	void SetEnabled(bool e) 
	{ 
		m_enabled = e;
	}

	// 寿命管理
	void Destroy()
	{
		m_alive = false;
	}

	bool IsAlive() const
	{
		return m_alive;
	}
};