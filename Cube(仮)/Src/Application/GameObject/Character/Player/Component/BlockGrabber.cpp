#include "BlockGrabber.h"
#include "../../../Block/NormalBlock/NormalBlock.h"
#include "../../../Block/BlockGridManager.h"
#include "../../../../Scene/SceneManager.h"

void BlockGrabber::Update(const Math::Vector3& playerPos, const Math::Matrix& playerRotMat)
{
	HandleGrabAndDrop(playerPos, playerRotMat);
	UpdateCarriedPos(playerPos, playerRotMat);
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
				if (targetPos.y < 0.0f) targetPos.y = 0.0f;

				Math::Vector3 snappedPos = BlockGridManager::SnapToGrid(targetPos);
				if (snappedPos.y < 0.0f) snappedPos.y = 0.0f;

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
				rayInfo.m_pos = playerPos + Math::Vector3(0, 1.5f, 0);
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

	Math::Vector3 eyePos = playerPos + Math::Vector3(0, 1.5f, 0);
	Math::Vector3 holdOffset = Math::Vector3(0.0f, 0.0f, m_holdDistance);
	holdOffset = Math::Vector3::Transform(holdOffset, playerRotMat);

	Math::Vector3 targetPos = eyePos + holdOffset;
	if (targetPos.y < 0.0f) targetPos.y = 0.0f;

	m_spCarriedBlock->SetPos(targetPos);
}
