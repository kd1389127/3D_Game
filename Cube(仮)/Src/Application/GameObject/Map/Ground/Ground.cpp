#include "Ground.h"

void Ground::Init(const std::string& modelPath, float scale)
{
	if (!m_spModel)
	{
		m_spModel = std::make_shared<KdModelWork>();
		m_spModel->SetModelData(modelPath);

		m_pCollider = std::make_unique<KdCollider>();
		m_pCollider->RegisterCollisionShape("Ground", m_spModel, KdCollider::TypeGround | KdCollider::TypeBump);
	}

	Math::Matrix sclaeMat = Math::Matrix::CreateScale(scale);
	m_mWorld = sclaeMat;
}

void Ground::DrawLit()
{
	if (!m_spModel) return;

	KdShaderManager::Instance().m_StandardShader.DrawModel(*m_spModel,m_mWorld);
}
