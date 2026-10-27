#pragma once

class MapBlock : public KdGameObject
{
public:
	// MagicCancel = 魔法ブロックを生成できない壁(見た目が黒い)
	enum class Shape { Full, Stair, MagicCancel };

	// 階段が「上っていく方向」(階段モデルは+X方向に上る向きで作る)
	enum class StairDir { PosX, NegX, PosZ, NegZ };

	MapBlock() {}
	~MapBlock() override {}

	void Init(const Math::Vector3& pos, Shape shape, StairDir dir = StairDir::PosX);
	void DrawUnLit() override;

	// 階段かどうか(Magicwandが「階段には生成させない」判定に使う)
	bool IsStair() const { return m_shape == Shape::Stair; }

	// カメラの行列から「映る範囲(視錐台)」の6つの面を作る。毎フレーム1回、カメラから呼ぶ
	static void UpdateFrustum(const Math::Matrix& view, const Math::Matrix& proj);

private:
	// モデルのパス(Shapeの並びと同じ順番にする)
	static constexpr int kShapeCount = 3;
	static constexpr const char* kModelPaths[kShapeCount] =
	{
		"Asset/Models/Map/MapBlock/MapBlock.gltf",
		"Asset/Models/Map/MapStair/MapStair.gltf",
		"Asset/Models/Map/MagicCancelBlock/MagicCancelBlock.gltf",
	};

	// 階段の明るさ(1.0で変化なし)。見ながら調整する
	static constexpr float kStairBrightness = 1.5f;

	std::shared_ptr<KdModelData> m_spModel = nullptr;
	Shape m_shape = Shape::Full;

	// 全ブロックで同じモデルを使いまわす(Shapeの番号で引く)
	static std::shared_ptr<KdModelData> s_spModels[kShapeCount];

	// 視錐台の6つの面(左・右・下・上・手前・奥)。x,y,zが面の向き、wが原点からの距離
	static Math::Vector4 s_planes[6];
};