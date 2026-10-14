#include "NormalBlock.h"

#include "../../../Scene/SceneManager.h"
void NormalBlock::Init(const Math::Vector3& pos)
{
	if (!m_spModel)
	{
		m_spModel = std::make_shared<KdModelWork>();
		m_spModel->SetModelData("Asset/Models/Block/MagicBlock/MagicBlock.gltf");
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
	// 体と重なって有効化を待っている間は、出現演出の最中でも毎フレーム確認する
	if (m_isWaitingEnable)
	{
		TryEnableCollider();
	}

	if (m_isEmerging)
	{
		m_emergeFrame++;

		// 段差演出用の開始遅延中(m_emergeFrameがまだマイナス)は、位置を動かさずに待つ
		if (m_emergeFrame < 0) return;

		// 経過フレームを 0.0～1.0 の進行度(t)に変換する
		float emergeRatio = std::clamp((float)m_emergeFrame / (float)m_emergeDuration, 0.0f, 1.0f);

		// EaseOutCubic：最初は速く、終わりに近づくほどゆっくり止まる動き方に変換する
		// (等速で動かすより、せり出す・生えてくる感じが自然に見える)
		float emergeEased = 1.0f - powf(1.0f - emergeRatio, 3.0f);

		// ディゾルブのしきい値は「1=見えない → 0=見える」なので、easedTを反転させる
		m_dissolveProgress = 1.0f - emergeEased;

		// アニメーションが完了したら後片付け
		if (emergeRatio >= 1.0f)
		{
			m_isEmerging = false;
			m_dissolveProgress = 0.0f;	// 完全に見える状態に
		}
		return;
	}

	if (m_isDismissing)
	{
		m_dismissFrame++;
		if (m_dismissFrame < 0) return;

		float dismissRatio = std::clamp((float)m_dismissFrame / (float)m_dismissDuration, 0.0f, 1.0f);
		float dismissEased = 1.0f - powf(1.0f - dismissRatio, 3.0f);
		m_dissolveProgress = dismissEased;		// 0(見える)→1(見えない)へ。StartEmergeと逆方向
		
		if (dismissRatio >= 1.0f)
		{
			m_isDismissing = false;
			Expire();						// アニメーション完了後、実際に消滅させる
		}
		return;
	}
}

void NormalBlock::PostUpdate()
{
	if (m_pDebugWire)
	{
		// BlockWall の可視化
		//m_pDebugWire->AddDebugBox(m_mWorld, Math::Vector3(0.5f, 0.5f, 0.5f), Math::Vector3::Zero, false, kRedColor);

		// BlockGround の可視化 (サイズ: 0.51f, 1.0f, 0.51f)
		//m_pDebugWire->AddDebugBox(m_mWorld, Math::Vector3(0.51f, 0.51f, 0.51f), Math::Vector3::Zero, false, kGreenColor);
	}
}

void NormalBlock::DrawLit()
{
	if (!m_spModel) return;

	// ----- ① プレビュー中(見本)なら、半透明の緑で描く -----
	if (m_isPreview)
	{
		// フレームワーク既存のAlphaブレンドに切り替え
		KdShaderManager::Instance().ChangeBlendState(KdBlendState::Alpha);
		if (m_isHighlighted)
		{
			// ハイライト中：点滅させたいので、時間経過でアルファを揺らす
			m_blinkTimer++;
			float blink = (sinf(m_blinkTimer * 0.2f) * 0.5f + 0.5f); //0.0～1.0を往復

			Math::Color previewColor(0.4f, 0.9f, 1.0f, 0.6f + blink * 0.4f);
			KdShaderManager::Instance().m_StandardShader.DrawModel(*m_spModel, m_mWorld, previewColor);
		}
		else
		{
			Math::Color previewColor(0.4f, 0.7f, 1.0f, 0.6f);
			KdShaderManager::Instance().m_StandardShader.DrawModel(*m_spModel, m_mWorld, previewColor);
		}
		// 変更前のブレンドステートに戻す
		KdShaderManager::Instance().UndoBlendState();
	}
	else if (m_isEmerging || m_isDismissing || m_dissolveProgress > 0.0f)
	{
		// ----- ② せり出しアニメーション中なら、ディゾルブで描く -----
		float range = 0.08f;							  // 境界のシャープさ(小さいほどくっきり)
		Math::Vector3 edgeColor = { 0.5f,1.0f,1.0f };	  // 発光色(水色)

		KdShaderManager::Instance().m_StandardShader.SetDissolve(m_dissolveProgress, &range, &edgeColor);
		KdShaderManager::Instance().m_StandardShader.DrawModel(*m_spModel, m_mWorld);
	}
	else
	{
		// ----- ③ 通常描画(確定済み、アニメーション完了後) -----
		if (m_isDeleteTargeted)
		{
			// 削除対象として狙われている間は赤く明滅させる
			m_blinkTimer++;
			float blink = (sinf(m_blinkTimer * 0.2f) * 0.5f + 0.5f);
			Math::Color deleteColor(1.0f, 0.25f + blink * 0.2f, 0.25f + blink * 0.2f, 1.0f);
			KdShaderManager::Instance().m_StandardShader.DrawModel(*m_spModel, m_mWorld, deleteColor);
		}
		else
		{
			KdShaderManager::Instance().m_StandardShader.DrawModel(*m_spModel, m_mWorld);
		}
	}
}

void NormalBlock::DrawUnLit()
{
	// ----- 狙われている面だけを光らせる枠を重ねて描く -----
	if (m_isFaceHighlighted)
	{
		DrawFaceHighlight();
	}
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
	m_dissolveProgress = 1.0f;

	// 見た目上のスタート地点(最終位置からdir方向の逆に1マス分ずらした位置)に、
	// 先にワープさせておいてからアニメーションで最終位置まで動かす
	SetPos(m_finalPos);

	if (m_pCollider)
	{
		// いったん全部切ってから、条件に合うものだけを有効にし直す
		m_pCollider->SetEnableAll(false);

		// 着弾した時点で体との重なりは確認済み(GenerateStackAtのTryStack)なので、
		// 出現演出の最初から当たり判定を有効にして、歩いて入れないようにする
		m_isWaitingEnable = true;
		TryEnableCollider();	// 重なっていなければ、壁も地面も今すぐ有効になる

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

void NormalBlock::StartDismiss(int delayFrames)
{
	m_isDismissing   = true;
	m_isWaitingEnable = false;
	m_dismissFrame	 = -delayFrames;
	m_dissolveProgress = 0.0f;		// 完全に見えている状態からスタート

	if (m_pCollider)
	{
		// 消滅演出中は当たり判定を切っておく(消えかけの状態で干渉させないため)
		m_pCollider->SetEnableAll(false);
	}
}

void NormalBlock::TryEnableCollider()
{
	if (!m_pCollider)
	{
		m_isWaitingEnable = false;
		return;
	}
	
	// 壁(横の押し出し)は先に有効にする
	// → 体が中にいても、Player側の壁判定が横へ押し出してくれる(縦には押さない)
	m_pCollider->SetEnable("BlockWall", true);

	// 地面(上に乗る判定)は、体と重なっていない時だけ有効にする
	// → 重なったまま有効にすると、足元のレイが内部の面を拾って上に持ち上げてしまうため
	if (BlockGridManager::Instance().IsBlockedByPlayer(m_finalPos)) return;

	m_pCollider->SetEnable("BlockGround", true);
	m_isWaitingEnable = false;
}

void NormalBlock::DrawFaceHighlight()
{
	if (!m_spModel) return;

	constexpr float half = 0.5f;		// ブロックのローカル半サイズ
	constexpr float thickness = 0.02f;	// 面方向だけ薄く潰す厚み

	Math::Vector3 faceNormal = m_highlightFaceNormal;

	// 面の中心(ローカル座標)＝ 法線方向に半サイズ + ごくわずか浮かせた位置
	Math::Vector3 localCenter = faceNormal * (half - 0.02f);

	// 面と平行な2軸はほぼブロック大(0.98)、法線方向だけ薄く潰す
	Math::Vector3 localScale
	(
		fabsf(faceNormal.x) > 0.5f ? thickness : 1.0f,
		fabsf(faceNormal.y) > 0.5f ? thickness : 1.0f,
		fabsf(faceNormal.z) > 0.5f ? thickness : 1.0f
	);

	// ブロック自身のワールド行列に乗せる
	Math::Matrix quadWorld =
		Math::Matrix::CreateScale(localScale) *
		Math::Matrix::CreateTranslation(localCenter) *
		m_mWorld;

	m_blinkTimer++;
	float blink = (sinf(m_blinkTimer * 0.2f) * 0.5f + 0.5f); //0.0～1.0を往復
	Math::Color faceColor(0.6f, 0.95f, 1.0f, 0.85f + blink * 0.15f);

	KdShaderManager::Instance().ChangeBlendState(KdBlendState::Add);
	KdShaderManager::Instance().m_StandardShader.DrawModel(*m_spModel, quadWorld, faceColor);
	KdShaderManager::Instance().UndoBlendState();
}
