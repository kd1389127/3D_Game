#pragma once

class KdEffekseerObject;

class Goal : public KdGameObject
{
public:
	Goal()						 {}
	~Goal()				override {}

	void Init(const Math::Vector3& pos);
	void Update()		override;
	void DrawUnLit()	override;

	// 近づいたかを調べる相手(プレイヤー)を渡してもらう
	void SetTarget(const std::shared_ptr<KdGameObject>& target) { m_wpTarget = target; }

private:
	std::shared_ptr<KdModelWork> m_spModel = nullptr;

	std::weak_ptr<KdGameObject>			m_wpTarget;			// プレイヤー
	std::weak_ptr<KdEffekseerObject>	m_wpEffect;			// 再生中の魔法陣
	bool  m_isNear = false;									// プレイヤーが近くにいるか
	float m_fadeRate = 0.0f;								// 出現の進み具合(0=消えている 1=最大)

	// ----- 光の柱の設定 -----
	static constexpr float kPillarWidth   = 8.0f;	// 柱の太さの倍率(魔法陣と同じくらいにする)
	static constexpr float kPillarHeight  = 10.0f;	// 柱の高さの倍率(壁の上面64より高くする)
	static constexpr float kPillarOffsetX = 0.0f;	// 柱の位置の調整(ゴールの中心からのずれ)
	static constexpr float kPillarOffsetY = -0.5f;	// 上げると柱が浮く、下げると床に埋まる
	static constexpr float kPillarOffsetZ = 0.0f;

	std::weak_ptr<KdEffekseerObject> m_wpPillar;			// 再生中の光の柱

};