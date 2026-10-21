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

	// Eキーの案内を出すステージか(GameSceneから渡す。Map2だけtrue)
	void SetShowHint(bool show) { m_showHint = show; }

private:

	void StartShake();

	KdTexture m_texNormal; // 通常時(青)
	KdTexture m_texDelete; // 削除モード中(赤)
	KdTexture m_texGrab;   // ギミックブロックを運んでいる、または狙っている時(緑)

	KdTexture m_texHintGrab;		// Eキーの案内(持ち上げる)
	KdTexture m_texHintRelease;		// Eキーの案内(置く)

	// Eキーの案内の位置と大きさ(Yは数字を大きくマイナスにするほど下がる)
	static constexpr float kHintY = -100.0f;
	static constexpr float kHintGrabW = 167.0f;
	static constexpr float kHintReleaseW = 109.0f;
	static constexpr float kHintH = 41.0f;
	bool m_showHint = false;

	std::weak_ptr<Player> m_wpPlayer;

	// 魔力切れ演出用
	int m_lastSeenFailId = 0;
	bool m_isShaking	 = false;
	int m_shakeFrame	 = 0;
	int m_shakeDuration  = 12;
	Math::Vector2 m_shakeOffset = Math::Vector2::Zero;
	float m_shakeScale   = 1.0f;

	int m_lastSeenDropBlockedId = 0;	// 前回見た「離せなかった回数」
};
