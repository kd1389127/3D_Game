#include "Switch.h"

#include "../../../Block/Grabbable.h"
#include "../../../../Scene/SceneManager.h"

namespace
{
	// スイッチの踏み板の大きさ(半分)。見た目に合わせてここを調整する
	constexpr float kPadHalfSize = 3.5f;
	// ブロックが踏み板の上に「乗っている」とみなす高さの許容範囲
	constexpr float kPressHeight = 3.0f;
}

void Switch::Init(const Math::Vector3& pos)
{
	if (!m_spModel)
	{
		m_spModel = std::make_shared<KdModelWork>();
		m_spModel->SetModelData("Asset/Models/Map/Switch/Switch.gltf");
	}

	SetScale(15.0f);
	SetPos(pos);
}

void Switch::Update()
{
	m_isActive = false;

	constexpr float blockHalf = BlockGridManager::GridSize * 0.5f;
	const Math::Vector3 padPos = GetPos();

	// ギミックブロックが1つでも、踏み板に少しでも重なって乗っていればオン
	for (auto& wp : BlockGridManager::Instance().GetFreeBlocks())
	{
		auto obj = wp.lock();
		if (!obj) continue;

		auto grabbable = std::dynamic_pointer_cast<IGrabbable>(obj);
		if (!grabbable) continue;
		if (grabbable->GetBlockKind() != BlockGridManager::BlockKind::GimmickKey) continue;
		if (grabbable->IsCarried()) continue; // 手に持っている間は押さない(置いたら作動)

		Math::Vector3 d = obj->GetPos() - padPos;

		// 横方向：踏み板とブロックが少しでも重なっているか
		bool overlapXZ = fabsf(d.x) < kPadHalfSize + blockHalf
			&& fabsf(d.z) < kPadHalfSize + blockHalf;

		// 縦方向：ブロックが踏み板の上(床の上)に乗っているか
		bool onPad = fabsf(d.y - blockHalf) < kPressHeight;

		if (overlapXZ && onPad)
		{
			m_isActive = true;
			break;
		}
	}
}

void Switch::DrawLit()
{
	if (!m_spModel) return;

	KdShaderManager::Instance().m_StandardShader.DrawModel(*m_spModel, m_mWorld);
}