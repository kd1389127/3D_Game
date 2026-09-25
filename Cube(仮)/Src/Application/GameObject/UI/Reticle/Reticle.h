#pragma once

class Player;

class Reticle : public KdGameObject
{
public:

	Reticle() {}
	~Reticle()			override {}

	void Init()			override;
	void Update()		override;
	void DrawSprite()	override;

	// 削除モード中かどうかを見るためにPlayerへの参照を持たせる
	void SetTarget(const std::shared_ptr<Player>& player) { m_wpPlayer = player; }

private:

	void StartShake();

	KdTexture m_texNormal; // 通常時(青)
	KdTexture m_texDelete; // 削除モード中(赤)
	std::weak_ptr<Player> m_wpPlayer;

	// 魔力切れ演出用
	int m_lastSeenFailId = 0;
	bool m_isShaking	 = false;
	int m_shakeFrame	 = 0;
	int m_shakeDuration  = 12;
	Math::Vector2 m_shakeOffset = Math::Vector2::Zero;
	float m_shakeScale   = 1.0f;

};
