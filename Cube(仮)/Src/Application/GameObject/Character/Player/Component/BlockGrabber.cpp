#include "BlockGrabber.h"
#include "../../../Block/NormalBlock/NormalBlock.h"
#include "../../../Block/BlockGridManager.h"
#include "../../../../Scene/SceneManager.h"
#include "../../../../main.h"

void BlockGrabber::Update(const Math::Vector3& playerPos, const Math::Matrix& playerRotMat)
{
	HandleGrabAndDrop(playerPos, playerRotMat);
	UpdateCarriedPos(playerPos, playerRotMat);
	HandleDistanceControl(); // ホイール処理
}

void BlockGrabber::HandleGrabAndDrop(const Math::Vector3& playerPos, const Math::Matrix& playerRotMat)
{
	if (GetAsyncKeyState('E') & 0x8000)
	{
		if (!m_eKeyFlg)
		{
			m_eKeyFlg = true;

			// 置く処理
			if (m_spCarriedBlock)
			{
				Math::Vector3 lookDir = playerRotMat.Backward();
				Math::Vector3 targetPos = playerPos + lookDir * m_holdDistance;
				constexpr float minY = BlockGridManager::GridSize * 0.5f;
				if (targetPos.y < minY) targetPos.y = minY;
				Math::Vector3 snappedPos = BlockGridManager::SnapToGrid(targetPos);
				if (snappedPos.y < minY) snappedPos.y = minY;

				if (!BlockGridManager::Instance().IsOccupied(snappedPos))
				{
					m_spCarriedBlock->SetPos(snappedPos);
					m_spCarriedBlock->SetCarried(false);
					BlockGridManager::Instance().Register(snappedPos);
					m_spCarriedBlock = nullptr;
				}
			}
			// 掴む処理
			else
			{
				KdCollider::RayInfo rayInfo;
				rayInfo.m_pos = playerPos;
				rayInfo.m_dir = playerRotMat.Backward();
				rayInfo.m_range = 30.0f;
				rayInfo.m_type = KdCollider::TypeBump | KdCollider::TypeGround;

				std::list<KdCollider::CollisionResult> resultList;
				std::shared_ptr<NormalBlock> targetBlock = nullptr;
				float minDist = rayInfo.m_range;

				for (auto& obj : SceneManager::Instance().GetObjList())
				{
					auto block = std::dynamic_pointer_cast<NormalBlock>(obj);
					if (!block || block->IsCarried()) continue;

					if (block->Intersects(rayInfo, &resultList))
					{
						for (auto& ret : resultList)
						{
							float dist = (ret.m_hitPos - rayInfo.m_pos).Length();
							if (dist < minDist)
							{
								minDist = dist;
								targetBlock = block;
							}
						}
						resultList.clear();
					}
				}

				if (targetBlock)
				{
					m_spCarriedBlock = targetBlock;
					m_holdDistance = 30.0f;
					Math::Vector3 snappedPos = BlockGridManager::SnapToGrid(targetBlock->GetPos());
					BlockGridManager::Instance().Unregister(snappedPos);
					m_spCarriedBlock->SetCarried(true);
				}
			}
		}
	}
	else
	{
		m_eKeyFlg = false;
	}
}

void BlockGrabber::UpdateCarriedPos(const Math::Vector3& playerPos, const Math::Matrix& playerRotMat)
{
	if (!m_spCarriedBlock) return;

	Math::Vector3 eyePos = playerPos;
	Math::Vector3 lookDir = playerRotMat.Backward();
	lookDir.Normalize();

	Math::Vector3 targetPos = eyePos + lookDir * m_holdDistance;

	KdCollider::RayInfo rayInfo;
	rayInfo.m_pos = eyePos;
	rayInfo.m_dir = lookDir;
	rayInfo.m_range = m_holdDistance;
	rayInfo.m_type = KdCollider::TypeBump | KdCollider::TypeGround;

	std::list<KdCollider::CollisionResult> resultList;
	float closestDist = m_holdDistance;
	bool isHit = false;

	for (auto& obj : SceneManager::Instance().GetObjList())
	{
		// ★【最重要】自分が今持っているブロック自身はレイ判定からスキップ
		if (obj == m_spCarriedBlock) continue;

		if (obj->Intersects(rayInfo, &resultList))
		{
			for (auto& ret : resultList)
			{
				float dist = (ret.m_hitPos - rayInfo.m_pos).Length();
				if (dist < closestDist)
				{
					closestDist = dist;
					isHit = true;
				}
			}
			resultList.clear();
		}
	}

	// 他の壁やブロックと交差する場合の補正
	if (isHit)
	{
		constexpr float blockRadius = BlockGridManager::GridSize; // 8.0f
		float adjustedDist = std::max(0.0f, closestDist - blockRadius);
		targetPos = eyePos + lookDir * adjustedDist;
	}

	// 最低高度の制限
	constexpr float minY = BlockGridManager::GridSize * 0.5f;
	if (targetPos.y < minY)
	{
		targetPos.y = minY;
	}

	m_spCarriedBlock->SetPos(targetPos);
}

void BlockGrabber::HandleDistanceControl()
{
	if (!m_spCarriedBlock) return;

	// Application からホイールの回転量を取得
	int wheelValue = Application::Instance().GetMouseWheelValue();

	if (wheelValue != 0)
	{
		// ホイール値を 120 (WHEEL_DELTA) で割って正規化
		float scrollDelta = static_cast<float>(wheelValue) / 120.0f;

		// 上回しで遠く(奥)、下回しで近く(手前)へ移動
		m_holdDistance += scrollDelta * 2.5f; // 2.5f は移動スピードの設定

		// 距離の制限（例: 10.0f ～ 60.0f の範囲に収める）
		m_holdDistance = std::clamp(m_holdDistance, 10.0f, 60.0f);
	}
}
