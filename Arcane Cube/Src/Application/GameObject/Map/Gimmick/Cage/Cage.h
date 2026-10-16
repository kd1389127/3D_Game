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

private:

	std::shared_ptr<KdModelWork> m_spModel = nullptr;
	std::vector<std::shared_ptr<Switch>> m_switches;

	Math::Vector3 m_basePos = {};
	float m_maxHeight		= 0.0f;
	float m_currentHeight	= 0.0f;
	float m_speed			= 0.2f;	// 1フレームあたりの移動量(Playerのm_moveSpeed等に合わせた感覚の値)
};