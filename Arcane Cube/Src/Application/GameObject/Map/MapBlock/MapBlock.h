#pragma once

class MapBlock : public KdGameObject
{
public:
	enum class Shape { Full, Stair };

	// 階段が「上っていく方向」(階段モデルは+X方向に上る向きで作る)
	enum class StairDir { PosX, NegX, PosZ, NegZ };

	MapBlock(){}
	~MapBlock() override{}

	void Init(const Math::Vector3& pos, Shape shape,StairDir dir = StairDir::PosX);
	void DrawLit() override;

private:
	std::shared_ptr<KdModelData> m_spModel = nullptr;

	// 全ブロックで同じモデルを使いまわす
	static std::shared_ptr<KdModelData> s_spFull;
	static std::shared_ptr<KdModelData> s_spStair;

};