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
	void DrawUnLit() override;

	// 階段かどうか(Magicwandが「階段には生成させない」判定に使う)
	bool IsStair() const { return m_isStair; }

	// カメラの行列から「映る範囲(視錐台)」の6つの面を作る。毎フレーム1回、カメラから呼ぶ
	static void UpdateFrustum(const Math::Matrix& view, const Math::Matrix& proj);

private:
	std::shared_ptr<KdModelData> m_spModel = nullptr;

	// 全ブロックで同じモデルを使いまわす
	static std::shared_ptr<KdModelData> s_spFull;
	// 視錐台の6つの面(左・右・下・上・手前・奥)。x,y,zが面の向き、wが原点からの距離
	static Math::Vector4 s_planes[6];

	static std::shared_ptr<KdModelData> s_spStair;
	bool m_isStair = false;
};