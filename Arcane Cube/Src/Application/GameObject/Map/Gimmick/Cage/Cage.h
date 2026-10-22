#pragma once

class Switch;

// Switchの状態を監視し、オンなら上昇/オフなら下降する鉄格子
class Cage : public KdGameObject
{
public:

	Cage() {}
	~Cage() override {}

	void Init(const std::vector<std::shared_ptr<Switch>>& switches,
		const Math::Vector3& basePos, const Math::Vector3& scale,
		float maxHeight, float speed, float yawDeg = 90.0f);	


	void Update() override;
	void DrawLit() override;

	// ブロック(中心center・半分の大きさhalf)が、檻の立っている範囲に重なるか
	bool OverlapsZone(const Math::Vector3& center, float half) const;

private:

	std::shared_ptr<KdModelWork> m_spModel = nullptr;
	std::vector<std::shared_ptr<Switch>> m_switches;

	Math::Vector3 m_basePos = {};
	float m_maxHeight		= 0.0f;
	float m_currentHeight	= 0.0f;
	float m_speed			= 0.2f;	// 1フレームあたりの移動量(Playerのm_moveSpeed等に合わせた感覚の値)
	bool  m_wasActive		= false;// 前のフレームでスイッチが全部オンだったか(開閉の始まりを見つけるため)

	Math::Vector3 m_zoneMin = {};	// 檻が立っている範囲(閉じた時の位置)
	Math::Vector3 m_zoneMax = {};
};