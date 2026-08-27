#include "GimmickBlock.h"

#include "../../../Scene/SceneManager.h"

void GimmickBlock::Init(const Math::Vector3& pos)
{
	if (!m_spModel)
	{
		m_spModel = std::make_shared<KdModelWork>();
		// TODO: 実際のギミック専用ブロックのモデルパスに置き換える
		m_spModel->SetModelData("Asset/Models/Block/WoodenBox/Wooden_Box.gltf");
	}

	if (!m_pDebugWire)
	{
		m_pDebugWire = std::make_unique<KdDebugWireFrame>();
	}

	if (!m_pCollider)
	{
		m_pCollider = std::make_unique<KdCollider>();

		// NormalBlockと同じ構成：壁用(横押し出し)と地面用(上に乗る判定)を分けて登録
		DirectX::BoundingBox wallBox;
		wallBox.Center = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
		wallBox.Extents = DirectX::XMFLOAT3(0.5f, 0.5f, 0.5f);
		m_pCollider->RegisterCollisionShape("BlockWall", wallBox, KdCollider::TypeBump);

		DirectX::BoundingBox groundBox;
		groundBox.Center = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
		groundBox.Extents = DirectX::XMFLOAT3(0.51f, 0.51f, 0.51f);
		m_pCollider->RegisterCollisionShape("BlockGround", groundBox, KdCollider::TypeGround);
	}

	SetPos(pos);
	SetScale(8.0f);
}

void GimmickBlock::PostUpdate()
{
	if (m_pDebugWire)
	{
		//m_pDebugWire->AddDebugBox(m_mWorld, Math::Vector3(0.5f, 0.5f, 0.5f), Math::Vector3::Zero, false, kRedColor);
	}
}

void GimmickBlock::DrawLit()
{
	if (!m_spModel) return;

	KdShaderManager::Instance().m_StandardShader.DrawModel(*m_spModel, m_mWorld);
}

void GimmickBlock::SetCarried(bool isCarried)
{
	m_isCarried = isCarried;

	if (m_pCollider)
	{
		m_pCollider->SetEnableAll(!isCarried);
	}
}
