#include "NormalBlock.h"

void NormalBlock::Init(const Math::Vector3& pos)
{
	if (!m_spModel)
	{
		m_spModel = std::make_shared<KdModelWork>();
		m_spModel->SetModelData("Asset/Models/Block/RockBlock/RockBlock.gltf");
	}

	if (!m_pDebugWire)
	{
		m_pDebugWire = std::make_unique<KdDebugWireFrame>();
	}

	if (!m_pCollider)
	{
		m_pCollider = std::make_unique<KdCollider>();

		// ★壁用(横の押し出し)：これまで通り体に合わせて少し狭め
		DirectX::BoundingBox wallBox;
		wallBox.Center = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
		wallBox.Extents = DirectX::XMFLOAT3(0.5f, 0.5f, 0.5f);
		m_pCollider->RegisterCollisionShape("BlockWall", wallBox, KdCollider::TypeBump);

		// ★地面用(上に乗る判定)：見た目の端(0.5=グリッド半分)まで、
		//   境目の隙間対策で気持ち広め(0.51)にしてブロック同士を確実に繋げる
		DirectX::BoundingBox groundBox;
		groundBox.Center = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
		groundBox.Extents = DirectX::XMFLOAT3(0.51f, 0.51f, 0.51f);
		m_pCollider->RegisterCollisionShape("BlockGround", groundBox, KdCollider::TypeGround);
	}

	// せり出しアニメーションの「最終着地点」として先に覚えておく
	// (StartEmergeが呼ばれない通常のInitだけで終わった場合は、この位置にそのまま置かれる)
	m_finalPos = pos;
	SetPos(pos);
	SetScale(8.0f);
}

// ===================================================
// 毎フレーム呼ばれる：せり出しアニメーション中だけ座標を更新する
// (アニメーションしていない間は何もしない)
// ===================================================
void NormalBlock::Update()
{
	if (!m_isEmerging) return;

	m_emergeFrame++;

	// 段差演出用の開始遅延中(m_emergeFrameがまだマイナス)は、位置を動かさずに待つ
	if (m_emergeFrame < 0) return;

	// 経過フレームを 0.0～1.0 の進行度(t)に変換する
	float t = std::clamp((float)m_emergeFrame / (float)m_emergeDuration, 0.0f, 1.0f);

	// EaseOutCubic：最初は速く、終わりに近づくほどゆっくり止まる動き方に変換する
	// (等速で動かすより、せり出す・生えてくる感じが自然に見える)
	float easedT = 1.0f - powf(1.0f - t, 3.0f);

	// スタート地点：最終位置から「せり出してくる方向の逆」に1マス分ずらした位置
	// (例：上方向にせり出すなら、1マス分下＝地中に埋まった状態からスタート)
	Math::Vector3 startPos = m_finalPos - m_emergeDir * BlockGridManager::GridSize;

	// startPos → m_finalPos へ、easedTの割合で線形補間した座標に移動させる
	SetPos(Math::Vector3::Lerp(startPos, m_finalPos, easedT));

	// アニメーションが完了したら後片付け
	if (t >= 1.0f)
	{
		m_isEmerging = false;

		if (m_pCollider)
		{
			// せり出し切ってから当たり判定を有効にする
			// (アニメ中に当たり判定が効くと、途中の浮いた状態でプレイヤーを押し出してしまうため)
			m_pCollider->SetEnableAll(true);
		}
	}
}

void NormalBlock::PostUpdate()
{
	if (m_pDebugWire)
	{
		// BlockWall の可視化
		m_pDebugWire->AddDebugBox(m_mWorld, Math::Vector3(0.5f, 0.5f, 0.5f), Math::Vector3::Zero, false, kRedColor);

		// BlockGround の可視化 (サイズ: 0.51f, 1.0f, 0.51f)
		//m_pDebugWire->AddDebugBox(m_mWorld, Math::Vector3(0.51f, 0.51f, 0.51f), Math::Vector3::Zero, false, kGreenColor);
	}
}

void NormalBlock::DrawLit()
{
	if (!m_spModel) return;

	KdShaderManager::Instance().m_StandardShader.DrawModel(*m_spModel, m_mWorld);
}

void NormalBlock::SetCarried(bool isCarried)
{
	m_isCarried = isCarried;

	if (m_pCollider)
	{
		// 持ち上げ中はコライダーを無効化、置いたら有効化
		m_pCollider->SetEnableAll(!isCarried);
	}
}

// ===================================================
// せり出しアニメーションを開始する
// ===================================================
void NormalBlock::StartEmerge(int delayFrames, const Math::Vector3& dir)
{
	m_isEmerging = true;
	m_emergeFrame = -delayFrames; // マイナスからスタートし、0になった瞬間から実際に動き始める
	m_emergeDir = dir;

	// 見た目上のスタート地点(最終位置からdir方向の逆に1マス分ずらした位置)に、
	// 先にワープさせておいてからアニメーションで最終位置まで動かす
	Math::Vector3 startPos = m_finalPos - dir * BlockGridManager::GridSize;
	SetPos(startPos);

	if (m_pCollider)
	{
		// アニメーション中は当たり判定を切っておく(浮いた/埋まった状態で干渉させないため)
		m_pCollider->SetEnableAll(false);
	}
}

void NormalBlock::SetPreview(bool isPreview)
{
	m_isPreview = isPreview;

	if (m_pCollider)
	{
		m_pCollider->SetEnableAll(!isPreview); // プレビュー中は当たらないように
	}
}