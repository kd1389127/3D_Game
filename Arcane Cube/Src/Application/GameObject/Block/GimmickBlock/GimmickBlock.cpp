#include "GimmickBlock.h"

#include "../../../Scene/SceneManager.h"

void GimmickBlock::Init(const Math::Vector3& pos)
{
	if (!m_spModel)
	{
		m_spModel = std::make_shared<KdModelWork>();
		// TODO: 実際のギミック専用ブロックのモデルパスに置き換える
		m_spModel->SetModelData("Asset/Models/Block/WoodenBox/Wooden_Box.gltf");
	}

	if (!m_pDebugWire)
	{
		m_pDebugWire = std::make_unique<KdDebugWireFrame>();
	}

	if (!m_pCollider)
	{
		m_pCollider = std::make_unique<KdCollider>();

		// NormalBlockと同じ構成：壁用(横押し出し)と地面用(上に乗る判定)を分けて登録
		DirectX::BoundingBox wallBox;
		wallBox.Center = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
		wallBox.Extents = DirectX::XMFLOAT3(0.5f, 0.5f, 0.5f);
		m_pCollider->RegisterCollisionShape("BlockWall", wallBox, KdCollider::TypeBump);

		DirectX::BoundingBox groundBox;
		groundBox.Center = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
		groundBox.Extents = DirectX::XMFLOAT3(0.51f, 0.51f, 0.51f);
		m_pCollider->RegisterCollisionShape("BlockGround", groundBox, KdCollider::TypeGround);
	}

	m_startPos = pos;
	SetPos(pos);
	SetScale(8.0f);
}

void GimmickBlock::Update()
{
	// 持ち運び中は、位置をBlockGrabber側が決めているので落下処理はしない
	if (m_isCarried) return;

	constexpr float gravity = 0.04f;
	constexpr float maxSpeed  = 3.0f;
	constexpr float halfSize  = BlockGridManager::GridSize * 0.5f;
	constexpr float footInset = 3.0f;	// 足元の4隅のレイを、中心から少し内側にずらす量

	m_fallSpeed = std::min(m_fallSpeed + gravity, maxSpeed);

	Math::Vector3 pos = GetPos();

	// 足元(中心＋4隅の計5本)から真下にレイを飛ばして、地面を探す
	// 4隅も使うのは、ブロックが足場の端からはみ出した時に宙に浮かないようにするため
	const float offX[5] = { 0.0f,  footInset, -footInset,  footInset, -footInset };
	const float offZ[5] = { 0.0f,  footInset,  footInset, -footInset, -footInset };

	bool landed = false;
	float landY = -FLT_MAX; // 見つかった地面のうち、一番高い位置

	std::list<KdCollider::CollisionResult> resultList;

	for (int i = 0; i < 5; ++i)
	{
		KdCollider::RayInfo ray;
		ray.m_pos = pos + Math::Vector3(offX[i], 0.0f, offZ[i]);
		ray.m_dir = Math::Vector3::Down;
		ray.m_range = halfSize + m_fallSpeed; // 中心から足元までの長さ＋今フレームに落ちる分
		ray.m_type = KdCollider::TypeGround;

		for (auto& obj : SceneManager::Instance().GetObjList())
		{
			if (obj.get() == this) continue; // 自分自身には当たらない

			resultList.clear();
			if (!obj->Intersects(ray, &resultList)) continue;

			for (auto& ret : resultList)
			{
				if (ret.m_hitPos.y > landY)
				{
					landY = ret.m_hitPos.y;
					landed = true;
				}
			}
		}
	}

	static int s_frame = 0;
	if (++s_frame % 20 == 0)
	{
		KdDebugGUI::Instance().AddLog("[Gimmick] y=%.2f speed=%.2f landed=%d landY=%.2f carried=%d\n",
			pos.y, m_fallSpeed, landed, landY, m_isCarried);
	}

	if (landed)
	{
		// 地面の上にぴったり乗せて、落下速度をリセット
		pos.y = landY + halfSize;
		m_fallSpeed = 0.0f;
	}
	else
	{
		pos.y -= m_fallSpeed;
	}

	// 奈落に落ちたら初期位置に戻す(詰み防止)
	if (pos.y < m_startPos.y - 200.0f)
	{
		pos = m_startPos;
		m_fallSpeed = 0.0f;
	}

	SetPos(pos);
}

void GimmickBlock::DrawLit()
{
	if (!m_spModel) return;

	// 狙われている間だけ、緑にゆっくり点滅させる
	// (運んでいる間は、置く位置のプレビューを見やすくするため、点滅しない)
	if (m_isGrabTargeted)
	{
		m_blinkTimer++;
		float blink = (sinf(m_blinkTimer * 0.1f) * 0.5f + 0.5f); // 0.0～1.0を往復(赤より遅い)
		Math::Color grabColor(0.4f + blink * 0.3f, 1.0f, 0.4f + blink * 0.3f, 1.0f);
		KdShaderManager::Instance().m_StandardShader.DrawModel(*m_spModel, m_mWorld, grabColor);
		return;
	}

	KdShaderManager::Instance().m_StandardShader.DrawModel(*m_spModel, m_mWorld);
}

void GimmickBlock::SetCarried(bool isCarried)
{
	m_isCarried = isCarried;
	m_fallSpeed = 0.0f;			// 掴んだ/離した瞬間は落下速度をリセット

	if (m_pCollider)
	{
		m_pCollider->SetEnableAll(!isCarried);
	}
}
