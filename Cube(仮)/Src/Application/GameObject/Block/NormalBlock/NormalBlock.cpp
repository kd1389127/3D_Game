#include "NormalBlock.h"

void NormalBlock::Init(const Math::Vector3& pos)
{
	if (!m_spModel)
	{
		m_spModel = std::make_shared<KdModelWork>();
		m_spModel->SetModelData("Asset/Models/Block/WoodenBox/Wooden_Box.gltf");

		m_pCollider = std::make_unique<KdCollider>();
		m_pCollider->RegisterCollisionShape("NormalBlock", m_spModel, KdCollider::TypeGround | KdCollider::TypeBump);
	}

	SetPos(pos);
	SetScale(8.0f);
}

void NormalBlock::DrawLit()
{
	if (!m_spModel) return;

	KdShaderManager::Instance().m_StandardShader.DrawModel(*m_spModel, m_mWorld);
}
