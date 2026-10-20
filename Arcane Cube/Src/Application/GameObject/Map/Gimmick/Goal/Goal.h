#pragma once

class KdEffekseerObject;

class Goal : public KdGameObject
{
public:
	Goal()						 {}
	~Goal()				override {}

	void Init(const Math::Vector3& pos, bool _showMagic = true);
	void Update()		override;
	void DrawUnLit()	override;

	// 近づいたかを調べる相手(プレイヤー)を渡してもらう
	void SetTarget(const std::shared_ptr<KdGameObject>& target) { m_wpTarget = target; }

private:
	std::shared_ptr<KdModelWork> m_spModel = nullptr;

	std::weak_ptr<KdGameObject>			m_wpTarget;		// プレイヤー
	std::weak_ptr<KdEffekseerObject>	m_wpEffect;		// 再生中の魔法陣
	bool  m_isNear = false;								// プレイヤーが近くにいるか
	float m_fadeRate = 0.0f;							// 出現の進み具合(0=消えている 1=最大)
};