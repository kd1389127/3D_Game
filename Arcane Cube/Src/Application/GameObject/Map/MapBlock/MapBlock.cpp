#include "MapBlock.h"

std::shared_ptr<KdModelData> MapBlock::s_spModels[MapBlock::kShapeCount] = {};
Math::Vector4 MapBlock::s_planes[6] = {};

void MapBlock::Init(const Math::Vector3& pos, Shape shape, StairDir dir)
{
	m_shape = shape;

	// 形の番号でモデルを選ぶ(まだ読んでいなければ、ここで1回だけ読む)
	const int index = static_cast<int>(shape);
	auto& spShared = s_spModels[index];
	if (!spShared)
	{
		spShared = std::make_shared<KdModelData>();
		spShared->Load(kModelPaths[index]);
	}
	m_spModel = spShared;

	m_pCollider = std::make_unique<KdCollider>();

	// ---- 階段: 見た目はL字、判定はモデル内の斜面(COL_Stair)1枚 ----
	if (shape == Shape::Stair)
	{
		m_pCollider->RegisterCollisionShape("MapStairSlope", m_spModel,
			KdCollider::TypeGround | KdCollider::TypeBump);

		// モデルは「+X方向に上る」向き。上りたい方向に合わせてY軸回転させる
		// (向きが期待と逆になったら、この4つの角度を入れ替える)
		float yaw = 0.0f;
		switch (dir)
		{
		case MapBlock::StairDir::PosX:
			yaw = 0.0f;
			break;
		case MapBlock::StairDir::NegX:
			yaw = DirectX::XM_PI;
			break;
		case MapBlock::StairDir::PosZ:
			yaw = DirectX::XM_PI + DirectX::XM_PIDIV2;
			break;
		case MapBlock::StairDir::NegZ:
			yaw = DirectX::XM_PIDIV2;
			break;
		default:
			break;
		}
		m_mWorld = Math::Matrix::CreateScale(8.0f)
			* Math::Matrix::CreateRotationY(yaw)
			* Math::Matrix::CreateTranslation(pos);
		return;
	}

	// ---- Full・MagicCancel(見た目が違うだけで、判定は同じ) ----
	// 壁用(横の押し出し)
	DirectX::BoundingBox wallBox;
	wallBox.Center = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	wallBox.Extents = DirectX::XMFLOAT3(0.5f, 0.5f, 0.5f);
	m_pCollider->RegisterCollisionShape("MapBlockWall", wallBox, KdCollider::TypeBump);

	// 地面用(上に乗る判定。境目の隙間対策で少し広め)
	DirectX::BoundingBox groundBox;
	groundBox.Center = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	groundBox.Extents = DirectX::XMFLOAT3(0.51f, 0.5f, 0.51f);
	m_pCollider->RegisterCollisionShape("MapBlockGround", groundBox, KdCollider::TypeGround);

	SetPos(pos);
	SetScale(8.0f);
}

void MapBlock::UpdateFrustum(const Math::Matrix& view, const Math::Matrix& proj)
{
	// ワールド座標 → 画面の座標に一気に変換する行列
	// view は「ワールド座標をカメラから見た座標に変える行列」
	// proj は「それを画面用に押し縮める行列」
	const Math::Matrix m = view * proj;

	// この行列の列を足し引きすると、画面の端にあたる6つの面になる
	s_planes[0] = Math::Vector4(m._14 + m._11, m._24 + m._21, m._34 + m._31, m._44 + m._41); // 左
	s_planes[1] = Math::Vector4(m._14 - m._11, m._24 - m._21, m._34 - m._31, m._44 - m._41); // 右
	s_planes[2] = Math::Vector4(m._14 + m._12, m._24 + m._22, m._34 + m._32, m._44 + m._42); // 下
	s_planes[3] = Math::Vector4(m._14 - m._12, m._24 - m._22, m._34 - m._32, m._44 - m._42); // 上
	s_planes[4] = Math::Vector4(m._13, m._23, m._33, m._43);                                 // 手前
	s_planes[5] = Math::Vector4(m._14 - m._13, m._24 - m._23, m._34 - m._33, m._44 - m._43); // 奥

	// 面の向き(x,y,z)の長さを1にそろえる。そろえないと「距離」として使えない
	for (auto& p : s_planes)
	{
		const float len = sqrtf(p.x * p.x + p.y * p.y + p.z * p.z);
		if (len > 0.0f) p /= len;
	}
}

void MapBlock::DrawUnLit()
{
	if (!m_spModel)return;

	// 映る範囲の外にあるブロックは描かない
	// ブロック(1辺8)がすっぽり入る球の半径は約6.93。少し余裕を見て7にしてある
	const Math::Vector3 c = GetPos();
	constexpr float radius = 7.0f;
	for (const Math::Vector4& p : s_planes)
	{
		// 面までの距離。マイナスは「面の外側」。-radiusより外側なら、球ごと外にある
		if (p.x * c.x + p.y * c.y + p.z * c.z + p.w < -radius) return;
	}

	// 階段だけ明るさを変える(1.0で変化なし)
	Math::Color col = kWhiteColor;
	if (m_shape == Shape::Stair)
	{
		col = Math::Color(kStairBrightness, kStairBrightness, kStairBrightness, 1.0f);
	}
	KdShaderManager::Instance().m_StandardShader.DrawModel(*m_spModel, m_mWorld, col);
}