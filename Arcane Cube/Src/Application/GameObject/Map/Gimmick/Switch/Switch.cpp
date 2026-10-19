#include "Switch.h"

#include "../../../Block/Grabbable.h"
#include "../../../Block/GimmickBlock/GimmickBlock.h"

namespace
{
	// スイッチの踏み板の大きさ(半分)。見た目に合わせてここを調整する
	constexpr float kPadHalfSize = 3.5f;
	// ブロックが踏み板の上に「乗っている」とみなす高さの許容範囲
	constexpr float kPressHeight = 3.0f;
	// 作動した時に、踏み板が沈む深さ
	constexpr float kPressDepth = 0.8f;
	// 1フレームに沈む・戻る量(0.1なら約8フレームで沈みきる)
	constexpr float kPressSpeed = 0.1f;
	// ブロックが乗る高さ(床から台座の縁の上面まで)。見ながら調整する
	constexpr float kRideHeight = 1.0f;
}

void Switch::Init(const Math::Vector3& pos)
{
	if (!m_spModel)
	{
		m_spModel = std::make_shared<KdModelData>();
		m_spModel->Load("Asset/Models/Map/Pedestal/Pedestal.gltf");

		m_spPadModel = std::make_shared<KdModelData>();
		m_spPadModel->Load("Asset/Models/Map/Switch/Switch.gltf");
	}

	SetScale(15.0f);
	SetPos(pos);
}

void Switch::Update()
{
	const bool wasActive = m_isActive;	// 前のフレームの状態を覚えておく(次の行でfalseに戻す前に)
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

			// 作動しているブロックは、見た目を板の上に乗せる(位置は動かさない)
			if (auto block = std::dynamic_pointer_cast<GimmickBlock>(obj))
			{
				block->SetRideOffset(kRideHeight);
			}
			break;
		}
	}

	// 踏み板に乗った瞬間だけ鳴らす(前はオフで、今回オンになった時)
	if (m_isActive && !wasActive)
	{
		auto se = KdAudioManager::Instance().Play("Asset/Sounds/SE/Gimmick/Switch.wav", false);
		if (se) 
		{ 
			se->SetVolume(0.2f); 
			se->SetPitch(-0.5f);
		}
	}

	// 作動中は沈み、離れたら戻る
	const float target = m_isActive ? kPressDepth : 0.0f;
	m_pressDepth += std::clamp(target - m_pressDepth, -kPressSpeed, kPressSpeed);
}

void Switch::DrawLit()
{
	if (!m_spModel || !m_spPadModel) return;

	auto& shader = KdShaderManager::Instance().m_StandardShader;

	// 台座はそのまま描く
	shader.DrawModel(*m_spModel, m_mWorld);

	// 踏み板は、沈んだ量だけ下にずらして描く
	const Math::Matrix padMat = m_mWorld * Math::Matrix::CreateTranslation(0.0f, -m_pressDepth, 0.0f);
	shader.DrawModel(*m_spPadModel, padMat);
}