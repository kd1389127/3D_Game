#include "BlockDestroyer.h"
#include "../../../Block/NormalBlock/NormalBlock.h"
#include "../../../Block/BlockGridManager.h"
#include "../../../Magic/MagicManager.h"
#include "../../../../Scene/SceneManager.h"
#include "../../../Collision/Collision.h"

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

	// レイの道の近くのものだけ集める
	std::vector<KdGameObject*> candidates;
	CollisionFilter::CollectAlongRay(rayInfo.m_pos, rayInfo.m_dir, rayInfo.m_range, candidates);

	KdGameObject* nearestObj = nullptr;	// 生ポインタで持つ(最後にshared_ptrへ戻す)
	float minDist = rayInfo.m_range;

	// listはループの外で1回だけ作り、使うたびにclear()する
	std::list<KdCollider::CollisionResult> resultList;

	for (KdGameObject* obj : candidates)
	{
		resultList.clear();
		if (!obj->Intersects(rayInfo, &resultList))continue;

		// キャストは「当たった後」だけ(今までは全オブジェクトにかけていた)
		auto* block = dynamic_cast<NormalBlock*>(obj);
		if (block && (block->IsPreview() || block->IsCarried())) continue;

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

	// 生ポインタから、管理側のshared_ptrを取り出す
	std::shared_ptr<NormalBlock> targetBlock;
	if (nearestObj)
	{
		targetBlock = std::dynamic_pointer_cast<NormalBlock>(nearestObj->shared_from_this());
	}

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

	// 削除の演出(ブロックの中心に出す。ディゾルブと同時に再生される)
	KdEffekseerManager::GetInstance().Play("Delete.efkefc", target->GetPos(), 3.0f);

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
