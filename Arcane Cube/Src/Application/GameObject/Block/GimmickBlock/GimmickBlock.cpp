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

	SetPos(pos);
	SetScale(8.0f);
}

void GimmickBlock::Update()
{
	if (!m_isFalling) return;

	constexpr float gravity  = 0.04f;
	constexpr float maxSpeed = 3.0f;
	m_fallSpeed = std::min(m_fallSpeed + gravity, maxSpeed);

	Math::Vector3 pos = GetPos();
	pos.y -= m_fallSpeed;

	// 着地位置に届いたら、ぴったりその位置に止めて、当たり判定を戻す
	if (pos.y <= m_fallTarget.y)
	{
		pos = m_fallTarget;
		m_isFalling = false;
		m_fallSpeed = 0.0f;

		// 予約を、本物のギミックブロックの登録に書き換える(スイッチが反応するのはここから)
		BlockGridManager::Instance().Register(m_fallTarget, GetBlockKind());

		if (m_pCollider)
		{
			m_pCollider->SetEnableAll(true);
		}
	}
	SetPos(pos);
}

void GimmickBlock::PostUpdate()
{
	if (m_pDebugWire)
	{
		//m_pDebugWire->AddDebugBox(m_mWorld, Math::Vector3(0.5f, 0.5f, 0.5f), Math::Vector3::Zero, false, kRedColor);
	}
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

	if (m_pCollider)
	{
		m_pCollider->SetEnableAll(!isCarried);
	}
}

// 空中で離された時に呼ばれる：着地位置まで落とし始める
void GimmickBlock::StartFall(const Math::Vector3& landingPos)
{
	m_isFalling = true;
	m_fallTarget = landingPos;
	m_fallSpeed = 0.0f;

	if (m_pCollider)
	{
		// 落下中は当たり判定を切っておく(途中の位置でプレイヤーを押し出さないため)
		m_pCollider->SetEnableAll(false);
	}
}
