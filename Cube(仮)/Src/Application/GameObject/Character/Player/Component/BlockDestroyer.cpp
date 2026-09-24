#include "BlockDestroyer.h"
#include "../../../Block/NormalBlock/NormalBlock.h"
#include "../../../Block/BlockGridManager.h"
#include "../../../Magic/MagicManager.h"
#include "../../../../Scene/SceneManager.h"
#include "../../../../main.h"

void BlockDestroyer::Update(const Math::Vector3& playerPos, const Math::Matrix& playerRotMat)
{
	HandleModeToggle();

	if (!m_isDeleteMode)
	{
		ClearTarget();
		return;
	}

	UpdateTargeting(playerPos, playerRotMat);
	HandleDeleteInput();
}

void BlockDestroyer::HandleModeToggle()
{
	bool qDownNow = (GetAsyncKeyState('Q') & 0x8000) != 0;

	if (qDownNow && !m_qKeyFlg)
	{
		m_qKeyFlg = true;
		m_isDeleteMode = !m_isDeleteMode;

		if (!m_isDeleteMode)
		{
			ClearTarget(); // モードを抜けた瞬間にハイライトを消す
		}
	}
	else if (!qDownNow)
	{
		m_qKeyFlg = false;
	}
}

void BlockDestroyer::UpdateTargeting(const Math::Vector3 & playerPos, const Math::Matrix & playerRotMat)
{
	KdCollider::RayInfo rayInfo;
	rayInfo.m_pos	= playerPos;
	rayInfo.m_dir	= playerRotMat.Backward();
	rayInfo.m_range = 50.0f;
	rayInfo.m_type  = KdCollider::TypeBump | KdCollider::TypeGround;

	std::shared_ptr<NormalBlock> targetBlock = nullptr;
	float minDist = rayInfo.m_range;


	for (auto& obj : SceneManager::Instance().GetObjList())
	{
		auto block = std::dynamic_pointer_cast<NormalBlock>(obj);

		// プレビュー中(まだ確定していない見本)や持ち運び中は削除対象にしない
		if (!block || block->IsPreview() || block->IsCarried()) continue;

		std::list<KdCollider::CollisionResult> resultList;
		if (!block->Intersects(rayInfo, &resultList))continue;

		for (auto& ret : resultList)
		{
			float dist = (ret.m_hitPos - rayInfo.m_pos).Length();
			if (dist < minDist)
			{
				minDist = dist;
				targetBlock = block;
			}
		}
	}

	// 前回のハイライトは一旦クリアしてから、新しい対象で作り直す
	ClearTarget();

	if (!targetBlock) return;

	int stackId = targetBlock->GetStackId();

	if (stackId < 0)
	{
		// スタックIDが振られていない場合(単発生成など)は単体だけを対象にする
		targetBlock->SetDeleteHighlight(true);
		m_wpTargetStack.push_back(targetBlock);
		return;
	}

	// 同じスタックIDを持つブロックを全部集める
	for (auto& obj : SceneManager::Instance().GetObjList())
	{
		auto block = std::dynamic_pointer_cast<NormalBlock>(obj);
		if (!block || block->IsPreview() || block->IsCarried()) continue;
		if (block->GetStackId() != stackId) continue;

		block->SetDeleteHighlight(true);
		m_wpTargetStack.push_back(block);
	}
}

void BlockDestroyer::HandleDeleteInput()
{
	bool leftDownNow = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
	bool leftPressed  = leftDownNow && !m_leftDownPrev;
	m_leftDownPrev = leftDownNow;

	if (!leftPressed || m_wpTargetStack.empty()) return;

	std::vector<std::shared_ptr<NormalBlock>> blocks;
	for (auto& wp : m_wpTargetStack)
	{
		if (auto b = wp.lock()) blocks.push_back(b);
	}

	// 先端(stackIndexが大きい方)から根元へ向かって消えるよう並べ替える
	std::sort(blocks.begin(), blocks.end(),
		[](const std::shared_ptr<NormalBlock>& a, const std::shared_ptr<NormalBlock>& b)
		{
			return a->GetStackIndex() > b->GetStackIndex();
		});

	constexpr int staggerFrames = 6;
	int delay = 0;

	for (auto& block : blocks)
	{
		BlockGridManager::Instance().Unregister(BlockGridManager::Instance().SnapToGrid(block->GetPos()));
		block->SetDeleteHighlight(false);
		block->StartDismiss(delay);
		delay += staggerFrames;
	}

	// スタック1つ削除につき魔力1回復(消費が段数に関係なく1固定なのと対称)
	MagicManager::Instance().Restore(1);

	m_wpTargetStack.clear();
}

void BlockDestroyer::ClearTarget()
{
	for (auto& wp : m_wpTargetStack)
	{
		if (auto b = wp.lock()) b->SetDeleteHighlight(false);
	}
	m_wpTargetStack.clear();
}
