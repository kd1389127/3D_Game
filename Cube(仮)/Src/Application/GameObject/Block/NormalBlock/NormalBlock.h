#pragma once
class NormalBlock : public KdGameObject
{
public:

	NormalBlock() {}
	~NormalBlock()		override {}

	void Init(const Math::Vector3& pos);
	void DrawLit()	override;

private:

	std::shared_ptr<KdModelWork> m_spModel = nullptr;

};