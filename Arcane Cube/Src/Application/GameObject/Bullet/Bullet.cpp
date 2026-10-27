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

	m_startPos = startPos;

	// 距離から飛行時間を決める(近いと短く、遠くても最長で頭打ち)
	const float distance = (m_targetPos - startPos).Length();
	const float speed = std::clamp(distance / m_maxFlightFrames, m_baseSpeed, m_maxSpeed);
	
	// その速度で届くまでのフレーム数(最低1フレーム)
	m_totalFrames = std::max(1, static_cast<int>(std::ceil(distance / speed)));
	m_flightFrame = 0;

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
	m_flightFrame++;
	const float t = static_cast<float>(m_flightFrame) / m_totalFrames; //0 -> 1で着弾

	// 着弾したとみなす(元の「残り距離 <= 速度」の判定を置き換え)
	if (t >= 1.0f)
	{
		SetPos(m_targetPos);

		// 斜めの角に当たった場合でも扱いやすいよう、法線を一番近い軸方向(±X/±Y/±Z)に丸める
		Math::Vector3 axisNormal = BlockGridManager::SnapNormalToAxis(m_hitNormal);

		if (m_onImpact)
		{
			// コールバックが渡されている場合：ブロック生成はこちらに任せる
			// (Magicwandの調整モードへ処理を引き継ぐ)
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

	// 一定の速さで直進する
	SetPos(Math::Vector3::Lerp(m_startPos, m_targetPos, t));
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