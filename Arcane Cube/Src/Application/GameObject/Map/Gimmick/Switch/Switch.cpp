#include "Switch.h"

#include "../../../Block/BlockGridManager.h"

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
	// 自分の真上のマスをチェックし、GimmickBlockが乗っているかどうかで
	// オン/オフを毎フレーム切り替える(乗せている間だけオン)
	// 地面に置かれたブロックが実際に収まるセル(半マス分上)をチェックする
	Math::Vector3 checkPos = GetPos() + Math::Vector3(0.0f, BlockGridManager::GridSize * 0.5f, 0.0f);
	Math::Vector3 snapped = BlockGridManager::Instance().SnapToGrid(checkPos);

	m_isActive = BlockGridManager::Instance().IsGimmickBlockAt(snapped);
}

void Switch::DrawLit()
{
	if (!m_spModel) return;

	KdShaderManager::Instance().m_StandardShader.DrawModel(*m_spModel, m_mWorld);
}