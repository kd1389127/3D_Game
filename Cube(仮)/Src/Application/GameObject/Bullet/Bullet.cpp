#include "Bullet.h"
#include "../Block/NormalBlock/NormalBlock.h"
#include "../../Scene/SceneManager.h"
#include "../Block/BlockGridManager.h"

void Bullet::Init(const Math::Vector3& startPos, const Math::Vector3& targetPos, const Math::Vector3& hitNormal,
	std::function<void(const Math::Vector3&, const Math::Vector3&)> onImpact)
{
	SetPos(startPos);
	m_targetPos = targetPos;

	m_hitNormal = hitNormal;
	m_hitNormal.Normalize();

	m_dir = m_targetPos - startPos;
	m_dir.Normalize();

	m_onImpact = onImpact; // 呼び出し元(Magicwandなど)から受け取った着弾時処理を保持しておく

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

	// 残り距離が1フレームの移動量以下になったら「着弾」とみなす
	if (m_speed >= remainDist)
	{
		SetPos(m_targetPos);

		// 斜めの角に当たった場合でも扱いやすいよう、法線を一番近い軸方向(±X/±Y/±Z)に丸める
		Math::Vector3 axisNormal = BlockGridManager::SnapNormalToAxis(m_hitNormal);

		if (m_onImpact)
		{
			// ★コールバックが渡されている場合：ブロック生成はこちらに任せる
			//   (Magicwandの調整モードへ処理を引き継ぐ)
			m_onImpact(m_targetPos, axisNormal);
		}
		else
		{
			// ----- コールバックが無い場合の従来処理(即時生成) -----
			// 着弾点から法線方向に「ほんの少し(半マス+α)」進めた点を取り、
			// 当たった面の「外側」のマスをそのまま狙う
			Math::Vector3 targetCellPos = m_targetPos + axisNormal * (BlockGridManager::GridSize * 0.5f + 0.01f);
			Math::Vector3 newCell = BlockGridManager::Instance().SnapToGrid(targetCellPos);

			if (!BlockGridManager::Instance().IsOccupied(newCell))
			{
				auto newBlock = std::make_shared<NormalBlock>();
				newBlock->Init(newCell);
				SceneManager::Instance().AddObject(newBlock);
				BlockGridManager::Instance().Register(newCell);
			}
		}

		m_isExpired = true; // 着弾処理が終わったので、この弾自体は消滅させる
		return;
	}

	// まだ届かない → 直進(発射方向×速度分だけ毎フレーム進める)
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