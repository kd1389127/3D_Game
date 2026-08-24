#pragma once

class Goal : public KdGameObject
{
public:
	Goal() {}
	~Goal() override {}

	void Init(const Math::Vector3& pos);
	void DrawLit() override;

private:
	std::shared_ptr<KdModelWork> m_spModel = nullptr;
};