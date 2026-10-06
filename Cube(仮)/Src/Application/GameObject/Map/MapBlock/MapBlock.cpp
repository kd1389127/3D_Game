#include "MapBlock.h"

std::shared_ptr<KdModelData> MapBlock::s_spFull = nullptr;
std::shared_ptr<KdModelData> MapBlock::s_spStair = nullptr;

void MapBlock::Init(const Math::Vector3& pos, Shape shape, StairDir dir)
{
	const bool isStair = (shape == Shape::Stair);

	auto& spShared = isStair ? s_spStair : s_spFull;
	if (!spShared)
	{
		spShared = std::make_shared<KdModelData>();

		spShared->Load(isStair
			? "Asset/Models/Map/MapStair/MapStair.gltf"
			: "Asset/Models/Map/MapBlock/MapBlock.gltf");
	}
	m_spModel = spShared;

	m_pCollider = std::make_unique<KdCollider>();

	// ---- 階段: 見た目はL字、判定はモデル内の斜面(COL_Stair)1枚 ----
	if (isStair)
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

void MapBlock::DrawLit()
{
	if (!m_spModel)return;
	KdShaderManager::Instance().m_StandardShader.DrawModel(*m_spModel, m_mWorld);
}
