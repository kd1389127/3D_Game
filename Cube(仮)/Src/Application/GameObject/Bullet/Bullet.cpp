#include "Bullet.h"
#include "../Block/NormalBlock/NormalBlock.h"
#include "../../Scene/SceneManager.h"
#include "../Block/BlockGridManager.h"

void Bullet::Init(const Math::Vector3& startPos, const Math::Vector3& targetPos, const Math::Vector3& hitNormal)
{
	SetPos(startPos);
	m_targetPos = targetPos;

	m_hitNormal = hitNormal;
	m_hitNormal.Normalize();

	m_dir = m_targetPos - startPos;
	m_dir.Normalize();

	if (!m_spModel)
	{
		m_spModel = std::make_shared<KdModelWork>();
		m_spModel->SetModelData("Asset/Models/Bullet/CreateBullet/CreateBullet.gltf");
	}

	SetScale(5.0f);
}

void Bullet::Update()
{
	Math::Vector3 toTarget = m_targetPos - GetPos();
	float         remainDist = toTarget.Length();

	// Bullet::Update() の着弾処理(旧: 半マスオフセット + スタック管理 → こちらに統一)
	if (m_speed >= remainDist)
	{
		SetPos(m_targetPos);

		// 1. 法線を一番近い軸方向 (±X, ±Y, ±Z) に丸める
		Math::Vector3 axisNormal = BlockGridManager::SnapNormalToAxis(m_hitNormal);

		// 2. 着弾点から法線方向に「ほんの少し（半マス未満）」だけ進めた点を取る
		//    （＝当たった面の「外側」のマスを直接狙う）
		Math::Vector3 targetCellPos = m_targetPos + axisNormal * (BlockGridManager::GridSize * 0.4f);

		// 3. その位置をそのまま最寄りのグリッドの中心にスナップさせる
		Math::Vector3 newCell = BlockGridManager::SnapToGrid(targetCellPos);

		// 4. 重複がなければブロックを生成
		if (!BlockGridManager::Instance().IsOccupied(newCell))
		{
			auto newBlock = std::make_shared<NormalBlock>();
			newBlock->Init(newCell);
			SceneManager::Instance().AddObject(newBlock);

			BlockGridManager::Instance().Register(newCell);
		}

		m_isExpired = true;
		return;
	}

	// まだ届かない → 直進
	SetPos(GetPos() + m_dir * m_speed);
}

void Bullet::DrawUnLit()
{
	if (!m_spModel) return;

	KdShaderManager::Instance().m_StandardShader.DrawModel(*m_spModel, m_mWorld);
}

void Bullet::DrawBright()
{
	if (!m_spModel) return;

	KdShaderManager::Instance().m_StandardShader.DrawModel(*m_spModel, m_mWorld);
}
