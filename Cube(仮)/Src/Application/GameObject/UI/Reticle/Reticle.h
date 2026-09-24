#pragma once

class Player;

class Reticle : public KdGameObject
{
public:

	Reticle() {}
	~Reticle()			override {}

	void Init()			override;
	void DrawSprite()	override;

	// 削除モード中かどうかを見るためにPlayerへの参照を持たせる
	void SetTarget(const std::shared_ptr<Player>& player) { m_wpPlayer = player; }

private:

	KdTexture m_texNormal; // 通常時(青)
	KdTexture m_texDelete; // 削除モード中(赤)
	std::weak_ptr<Player> m_wpPlayer;

};
