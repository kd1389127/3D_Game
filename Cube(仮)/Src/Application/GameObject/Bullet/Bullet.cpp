#include "Bullet.h"
#include "../Block/NormalBlock/NormalBlock.h"
#include "../../Scene/SceneManager.h"
#include "../Block/BlockGridManager.h"

void Bullet::Init(const Math::Vector3& startPos, const Math::Vector3& targetPos)
{
	SetPos(startPos);
	m_targetPos = targetPos;

	m_dir = m_targetPos - startPos;
	m_dir.Normalize();

	if (!m_spModel)
	{
		m_spModel = std::make_shared<KdModelWork>();
		m_spModel->SetModelData("Asset/Models/Bullet/Arrow.gltf");
	}

	SetScale(10.0f);
}

void Bullet::Update()
{
	Math::Vector3 toTarget = m_targetPos - GetPos();
	float         remainDist = toTarget.Length();

	// 今フレームで到達 or 追い越す場合 → 着弾点ピッタリで確定させる
	if (m_speed >= remainDist)
	{
		SetPos(m_targetPos);

		Math::Vector3 snappedPos = BlockGridManager::SnapToGrid(m_targetPos);

		// 既にそのマスにブロックがあれば生成しない
		if (!BlockGridManager::Instance().IsOccupied(snappedPos))
		{
			auto newBlock = std::make_shared<NormalBlock>();
			newBlock->Init(m_targetPos);
			SceneManager::Instance().AddObject(newBlock);
			
			BlockGridManager::Instance().Register(snappedPos);
		}

		m_isExpired = true;
		return;
	}

	// まだ届かない → 直進
	SetPos(GetPos() + m_dir * m_speed);
}

void Bullet::DrawLit()
{
	if (!m_spModel) return;
	KdShaderManager::Instance().m_StandardShader.DrawModel(*m_spModel, m_mWorld);
}
