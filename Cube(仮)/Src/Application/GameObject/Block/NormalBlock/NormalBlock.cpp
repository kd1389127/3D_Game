#include "NormalBlock.h"

void NormalBlock::Init(const Math::Vector3& pos)
{
	if (!m_spModel)
	{
		m_spModel = std::make_shared<KdModelWork>();
		m_spModel->SetModelData("Asset/Models/Block/WoodenBox/Wooden_Box.gltf");
	}

	if (!m_pCollider)
	{
		m_pCollider = std::make_unique<KdCollider>();

		// ★重要: TypeGround（床）と TypeBump（壁・衝突）の両方を指定する
		UINT collisionType = KdCollider::TypeGround | KdCollider::TypeBump;

		// モデルのメッシュ形状からコリジョンを作成・登録
		m_pCollider->RegisterCollisionShape
		(
			"BlockCollision",   // 登録名（任意の識別子）
			m_spModel,          // 対象のモデル
			collisionType       // 当たり判定のタイプ
		);

	}

	SetPos(pos);
	SetScale(8.0f);
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
