#pragma once
class NormalBlock : public KdGameObject
{
public:

	NormalBlock() {}
	~NormalBlock()		override {}

	void Init()		override;
	void DrawLit()	override;

private:

	std::shared_ptr<KdModelWork> m_spModel = nullptr;

};