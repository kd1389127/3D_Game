#pragma once

// GimmickBlockが真上のマスに乗っている間だけオンになるスイッチ
class Switch : public KdGameObject
{
public:

	Switch() {}
	~Switch() override {}

	void Init(const Math::Vector3& pos);
	void Update() override;
	void DrawLit() override;

	bool IsActive() const { return m_isActive; }

private:

	std::shared_ptr<KdModelData> m_spModel = nullptr;     // 台座(Pedestal)
	std::shared_ptr<KdModelData> m_spPadModel = nullptr;  // 踏み板(Switch)

	bool m_isActive = false;
	float m_pressDepth = 0.0f;  // 今の沈み量(0 = 元の高さ)

	Math::Vector3 m_debugCheckPos = Math::Vector3::Zero; //判定位置保持用
};