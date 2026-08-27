#include "Goal.h"

void Goal::Init(const Math::Vector3& pos)
{
	if (!m_spModel)
	{
		m_spModel = std::make_shared<KdModelWork>();
		// 仮モデル：とりあえず既存のモデルを流用してOK(例：NormalBlockのモデルなど)
		m_spModel->SetModelData("Asset/Models/Map/Goal/Goal.gltf");

		m_pCollider = std::make_unique<KdCollider>();
		// TypeEvent = イベント判定用(触れたら何か起きる、という用途)
		m_pCollider->RegisterCollisionShape("Goal", m_spModel, KdCollider::TypeEvent);
	}

	SetScale(10.0f);
	SetPos(pos);
}

void Goal::DrawUnLit()
{
	if (!m_spModel) return;

	KdShaderManager::Instance().m_StandardShader.DrawModel(*m_spModel, m_mWorld);
}

void Goal::DrawBright()
{
	if (!m_spModel) return;

	KdShaderManager::Instance().m_StandardShader.DrawModel(*m_spModel, m_mWorld);
}
