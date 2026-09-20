#include "WeaponBase.h"

void WeaponBase::Update()
{
	const std::shared_ptr<const KdGameObject> spParent = m_wpParent.lock();
	if (spParent)
	{
		Math::Matrix parentMat = spParent->GetMatrix();

		// スケール→振り(animMat)→傾き・位置(localMat)→親、の順に適用する
		// (スケールを回転より先にかけることで、非等方スケールによる歪みを防ぐ)
		m_mWorld = m_scaleMat * m_animMat * m_localMat * parentMat;
	}
}

void WeaponBase::DrawLit()
{
	if (!m_spModel) return;

	KdShaderManager::Instance().m_StandardShader.DrawModel(*m_spModel,m_mWorld);
}
