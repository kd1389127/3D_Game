#include "Goal.h"

void Goal::Init(const Math::Vector3& pos)
{
	if (!m_spModel)
	{
		m_spModel = std::make_shared<KdModelWork>();
		// 仮モデル：とりあえず既存のモデルを流用してOK(例：NormalBlockのモデルなど)
		m_spModel->SetModelData("Asset/Models/Block/WoodenBox/Wooden_Box.gltf");

		m_pCollider = std::make_unique<KdCollider>();
		// TypeEvent = イベント判定用(触れたら何か起きる、という用途)
		m_pCollider->RegisterCollisionShape("Goal", m_spModel, KdCollider::TypeEvent);
	}

	SetScale(3.0f);

	m_mWorld = Math::Matrix::CreateTranslation(pos);
}

void Goal::DrawLit()
{
	if (!m_spModel) return;

	KdShaderManager::Instance().m_StandardShader.DrawModel(*m_spModel, m_mWorld);
}