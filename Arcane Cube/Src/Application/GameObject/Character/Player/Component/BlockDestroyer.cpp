#include "BlockDestroyer.h"
#include "../../../Block/NormalBlock/NormalBlock.h"
#include "../../../Block/BlockGridManager.h"
#include "../../../Magic/MagicManager.h"
#include "../../../../Scene/SceneManager.h"

void BlockDestroyer::Update(const Math::Vector3& playerPos, const Math::Matrix& playerRotMat)
{
	UpdateTargeting(playerPos, playerRotMat);
}

void BlockDestroyer::UpdateTargeting(const Math::Vector3 & playerPos, const Math::Matrix & playerRotMat)
{
	KdCollider::RayInfo rayInfo;
	rayInfo.m_pos	= playerPos;
	rayInfo.m_dir	= playerRotMat.Backward();
	rayInfo.m_range = 50.0f;
	rayInfo.m_type  = KdCollider::TypeBump | KdCollider::TypeGround;

	std::shared_ptr<KdGameObject> nearestObj = nullptr;
	float minDist = rayInfo.m_range;

	for (auto& obj : SceneManager::Instance().GetObjList())
	{
		// プレビュー中(まだ確定していない見本)や持ち運び中のブロックだけ、障害物としても数えない
		auto block = std::dynamic_pointer_cast<NormalBlock>(obj);
		if (block && (block->IsPreview() || block->IsCarried())) continue;

		std::list<KdCollider::CollisionResult> resultList;
		if (!obj->Intersects(rayInfo, &resultList)) continue;

		for (auto& ret : resultList)
		{
			float dist = (ret.m_hitPos - rayInfo.m_pos).Length();
			if (dist < minDist)
			{
				minDist = dist;
				nearestObj = obj;
			}
		}
	}

	std::shared_ptr<NormalBlock> targetBlock = std::dynamic_pointer_cast<NormalBlock>(nearestObj);

	auto prevTarget = m_wpTarget.lock();
	if (prevTarget == targetBlock) return; // 対象が変わっていなければ何もしない

	if (prevTarget)  prevTarget->SetDeleteHighlight(false);
	if (targetBlock) targetBlock->SetDeleteHighlight(true);

	m_wpTarget = targetBlock;
}

bool BlockDestroyer::TryDeleteTarget()
{
	auto target = m_wpTarget.lock();
	if (!target) return false;

	BlockGridManager::Instance().Unregister(BlockGridManager::Instance().SnapToGrid(target->GetPos()));
	target->SetDeleteHighlight(false);
	target->StartDismiss(0);

	MagicManager::Instance().Restore(1); // ブロック1個=魔力1個

	m_wpTarget.reset();
	return true;
}

void BlockDestroyer::ClearTarget()
{
	if (auto target = m_wpTarget.lock())
	{
		target->SetDeleteHighlight(false);
	}
	m_wpTarget.reset();
}
