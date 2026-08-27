#include "NormalBlock.h"

void NormalBlock::Init(const Math::Vector3& pos)
{
	if (!m_spModel)
	{
		m_spModel = std::make_shared<KdModelWork>();
		//m_spModel->SetModelData("Asset/Models/Block/WoodenBox/Wooden_Box.gltf");
		m_spModel->SetModelData("Asset/Models/Block/RockBlock/RockBlock.gltf");
	}

	if (!m_pDebugWire)
	{
		m_pDebugWire = std::make_unique<KdDebugWireFrame>();
	}

	if (!m_pCollider)
	{
		m_pCollider = std::make_unique<KdCollider>();

		// ★壁用(横の押し出し)：これまで通り体に合わせて少し狭め
		DirectX::BoundingBox wallBox;
		wallBox.Center = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
		wallBox.Extents = DirectX::XMFLOAT3(0.5f, 0.5f, 0.5f);
		m_pCollider->RegisterCollisionShape("BlockWall", wallBox, KdCollider::TypeBump);
		

		// ★地面用(上に乗る判定)：見た目の端(0.5=グリッド半分)まで、
		//   境目の隙間対策で気持ち広め(0.51)にしてブロック同士を確実に繋げる
		DirectX::BoundingBox groundBox;
		groundBox.Center = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
		groundBox.Extents = DirectX::XMFLOAT3(0.51f, 0.51f, 0.51f);
		m_pCollider->RegisterCollisionShape("BlockGround", groundBox, KdCollider::TypeGround);
	}

	SetPos(pos);
	SetScale(8.0f);
}

void NormalBlock::PostUpdate()
{
	if (m_pDebugWire)
	{
		// BlockWall の可視化
		m_pDebugWire->AddDebugBox(m_mWorld, Math::Vector3(0.5f, 0.5f, 0.5f), Math::Vector3::Zero, false, kRedColor);
	
		// BlockGround の可視化 (サイズ: 0.51f, 1.0f, 0.51f)
		//m_pDebugWire->AddDebugBox(m_mWorld, Math::Vector3(0.51f, 0.51f, 0.51f), Math::Vector3::Zero, false, kGreenColor);
	}
}

void NormalBlock::DrawLit()
{
	if (!m_spModel) return;

	KdShaderManager::Instance().m_StandardShader.DrawModel(*m_spModel, m_mWorld);
}

void NormalBlock::SetCarried(bool isCarried)
{
	m_isCarried = isCarried;

	if (m_pCollider)
	{
		// 持ち上げ中はコライダーを無効化、置いたら有効化
		m_pCollider->SetEnableAll(!isCarried);
	}
}
