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

	std::shared_ptr<KdModelWork> m_spModel = nullptr;

	bool m_isActive = false;
};