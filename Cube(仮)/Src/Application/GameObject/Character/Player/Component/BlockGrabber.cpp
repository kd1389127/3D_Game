#include "BlockGrabber.h"
#include "../../../Block/NormalBlock/NormalBlock.h"
#include "../../../Block/BlockGridManager.h"
#include "../../../../Scene/SceneManager.h"
#include "../../../../main.h"

namespace
{
	// 指定した座標(pos)に、他のブロックが重なっていないかチェックする
	// ignoreBlock ＝ 今動かそうとしている本人のブロック(自分自身とは比較しない)
	bool OverlapsAnyBlock(const Math::Vector3& pos, const std::shared_ptr<NormalBlock>& ignoreBlock)
	{
		constexpr float blockSize = BlockGridManager::GridSize; // 8.0f

		for (auto& obj : SceneManager::Instance().GetObjList())
		{
			auto block = std::dynamic_pointer_cast<NormalBlock>(obj);
			// ブロックじゃない/自分自身/今持ち運び中のブロックは比較対象から除外
			if (!block || block == ignoreBlock || block->IsCarried()) continue;

			// XYZそれぞれの距離差が1マス分未満なら「重なっている」とみなす(簡易的な箱同士の判定)
			Math::Vector3 diff = pos - block->GetPos();
			if (fabsf(diff.x) < blockSize && fabsf(diff.y) < blockSize && fabsf(diff.z) < blockSize)
				return true;
		}
		return false;
	}

	// ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== =====
	// 持っているブロックを「安全な位置(oldPos)」から「目標位置(desiredPos)」へ移動させる関数
	// ただし、他のブロックに重なる移動は許可しない(すり抜け防止)
	// X→Z→Yの順に「その軸だけ動かしてみて、重ならなければ確定」を1軸ずつ試すことで、
	// 壁際を滑るように移動できるようにしている(いわゆる「スライド移動」)
	// ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== =====
	Math::Vector3 SlideMove(const Math::Vector3& oldPos, const Math::Vector3& desiredPos,
		const std::shared_ptr<NormalBlock>& ignoreBlock)
	{
		Math::Vector3 result = oldPos;

		// X軸だけ動かしてみて、重ならなければ確定
		Math::Vector3 tryX = result; tryX.x = desiredPos.x;
		if (!OverlapsAnyBlock(tryX, ignoreBlock)) result.x = desiredPos.x;

		// 次にZ軸だけ動かしてみて、重ならなければ確定
		Math::Vector3 tryZ = result; tryZ.z = desiredPos.z;
		if (!OverlapsAnyBlock(tryZ, ignoreBlock)) result.z = desiredPos.z;

		// 最後にY軸だけ動かしてみて、重ならなければ確定
		Math::Vector3 tryY = result; tryY.y = desiredPos.y;
		if (!OverlapsAnyBlock(tryY, ignoreBlock)) result.y = desiredPos.y;

		return result;
	}

	// ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== =====
	// 持っているブロックが、プレイヤー自身に近づきすぎないようにする関数
	// (ブロックを引き寄せすぎるとプレイヤーにめり込んでしまうのを防ぐ)
	// ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== =====
	void ClampMinDistanceFromPlayer(Math::Vector3& pos, const Math::Vector3& playerPos, const Math::Vector3& fallbackDir)
	{
		constexpr float minPlayerDist = BlockGridManager::GridSize; // 最低でも1マス分は離す

		Math::Vector3 toBlock = pos - playerPos;
		toBlock.y = 0.0f; // 上下は無視して、水平方向の距離だけで判定

		float dist = toBlock.Length();

		// 最低距離より近い場合は、方向はそのままで距離だけ最低値まで押し戻す
		if (dist < minPlayerDist)
		{
			Math::Vector3 dir;
			if (dist > 0.0001f)
			{
				dir = toBlock / dist; // 正規化(向きだけを取り出す)
			}
			else
			{
				// プレイヤーの真上/真下などで方向が定まらない場合の保険(見ている方向を代わりに使う)
				dir = fallbackDir;
			}

			pos.x = playerPos.x + dir.x * minPlayerDist;
			pos.z = playerPos.z + dir.z * minPlayerDist;
		}
	}
}

// 初期化：プレビュー用モデルの読み込みと、半透明描画のための設定(ブレンドステート)を準備
void BlockGrabber::Init()
{
	m_spPreviewModel = std::make_shared<KdModelWork>();
	m_spPreviewModel->SetModelData("Asset/Models/Block/RockBlock/RockBlock.gltf");

	// 半透明描画をするための設定(アルファブレンド)を作成
	// これがないと、持ち上げたブロックのプレビューを「透けた状態」で描画できない
	D3D11_BLEND_DESC blendDesc = {};
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	KdDirect3D::Instance().WorkDev()->CreateBlendState(&blendDesc, m_alphaBlendState.GetAddressOf());
}

// 毎フレームの更新：「掴む/離す」「持っているブロックの位置更新」「ホイールでの距離調整」をまとめて呼ぶ
void BlockGrabber::Update(const Math::Vector3& playerPos, const Math::Matrix& playerRotMat)
{
	HandleGrabAndDrop(playerPos, playerRotMat);
	UpdateCarriedPos(playerPos, playerRotMat);
	HandleDistanceControl(); // マウスホイール処理
}

// 今持っているブロックの座標を返す(何も持っていなければ原点を返す)
Math::Vector3 BlockGrabber::GetCarriedBlockPos() const
{
	return m_spCarriedBlock ? m_spCarriedBlock->GetPos() : Math::Vector3::Zero;
}

// 持ち上げ中のブロックを「置いたらここに置かれる」というプレビュー(半透明の見本)として描画する
void BlockGrabber::DrawPreview()
{
	if (!m_spCarriedBlock || !m_spPreviewModel) return;

	auto context = KdDirect3D::Instance().WorkDevContext();

	// 現在のブレンドステート(描画設定)を一旦退避しておく(あとで元に戻すため)
	Microsoft::WRL::ComPtr<ID3D11BlendState> prevBlendState;
	float prevBlendFactor[4];
	UINT prevSampleMask;
	context->OMGetBlendState(prevBlendState.GetAddressOf(), prevBlendFactor, &prevSampleMask);

	// 半透明用の設定に切り替える
	float blendFactor[4] = { 1,1,1,1 };
	context->OMSetBlendState(m_alphaBlendState.Get(), blendFactor, 0xFFFFFFFF);

	Math::Matrix mat = Math::Matrix::CreateScale(8.0f) * Math::Matrix::CreateTranslation(m_previewPos);

	// 置ける場所なら緑、置けない場所(何かと重なっている)なら赤で表示する
	Math::Color previewColor = m_previewValid
		? Math::Color(0.5f, 1.0f, 0.5f, 0.6f)  // 緑・半透明
		: Math::Color(1.0f, 0.3f, 0.3f, 0.6f); // 赤・半透明

	KdShaderManager::Instance().m_StandardShader.DrawModel(*m_spPreviewModel, mat, previewColor);

	// 描画設定を元に戻す(他の描画に影響を残さないため)
	context->OMSetBlendState(prevBlendState.Get(), prevBlendFactor, prevSampleMask);
}

// ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== =====
// Eキーで「掴む」「置く」を切り替える処理
// ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== =====
void BlockGrabber::HandleGrabAndDrop(const Math::Vector3& playerPos, const Math::Matrix& playerRotMat)
{
	if (GetAsyncKeyState('E') & 0x8000)
	{
		// キーを押しっぱなしで毎フレーム反応しないよう、押した瞬間だけ処理するためのフラグ
		if (!m_eKeyFlg)
		{
			m_eKeyFlg = true;

			// ---- すでに何か持っている場合 → 「置く」処理 ----
			if (m_spCarriedBlock)
			{
				Math::Vector3 targetPos = CalcTargetPos(playerPos, playerRotMat);

				constexpr float minY = BlockGridManager::GridSize * 0.5f;
				Math::Vector3 snappedPos = BlockGridManager::SnapToGrid(targetPos);
				if (snappedPos.y < minY) snappedPos.y = minY; // 地面より下に置かれないようにする

				// そのマスが空いていれば実際に設置する
				if (!BlockGridManager::Instance().IsOccupied(snappedPos))
				{
					m_spCarriedBlock->SetPos(snappedPos);
					m_spCarriedBlock->SetCarried(false);
					BlockGridManager::Instance().Register(snappedPos); // マスを「使用中」に登録
					m_spCarriedBlock = nullptr; // 手放す
				}
			}
			// ---- 何も持っていない場合 → 「掴む」処理 ----
			else
			{
				// プレイヤーの視線方向にレイ(見えない光線)を飛ばして、ブロックに当たるか調べる
				KdCollider::RayInfo rayInfo;
				rayInfo.m_pos = playerPos;
				rayInfo.m_dir = playerRotMat.Backward(); // カメラの前方向
				rayInfo.m_range = 30.0f; // これより遠いブロックは掴めない
				rayInfo.m_type = KdCollider::TypeBump | KdCollider::TypeGround;

				std::list<KdCollider::CollisionResult> resultList;
				std::shared_ptr<NormalBlock> targetBlock = nullptr;
				float minDist = rayInfo.m_range;

				// マップ上の全ブロックに対してレイが当たるかチェックし、一番近いブロックを探す
				for (auto& obj : SceneManager::Instance().GetObjList())
				{
					auto block = std::dynamic_pointer_cast<NormalBlock>(obj);
					if (!block || block->IsCarried()) continue; // ブロックでない/既に持ち運び中は除外

					if (block->Intersects(rayInfo, &resultList))
					{
						for (auto& ret : resultList)
						{
							float dist = (ret.m_hitPos - rayInfo.m_pos).Length();
							if (dist < minDist)
							{
								minDist = dist;
								targetBlock = block; // より近いブロックを見つけたら更新
							}
						}
						resultList.clear();
					}
				}

				// 見つかったブロックを「持っている状態」にする
				if (targetBlock)
				{
					m_spCarriedBlock = targetBlock;
					m_holdDistance = 30.0f; // 持ち上げた瞬間の保持距離(初期値)

					// マス目管理から「このマスはもう埋まっていない」ことにする(持ち上げたので)
					Math::Vector3 snappedPos = BlockGridManager::SnapToGrid(targetBlock->GetPos());
					BlockGridManager::Instance().Unregister(snappedPos);

					m_spCarriedBlock->SetCarried(true);
				}
			}
		}
	}
	else
	{
		// キーが離されたらフラグをリセット(次に押した時にまた反応できるようにする)
		m_eKeyFlg = false;
	}
}

// 持っているブロックの位置と、プレビュー(緑/赤の半透明表示)の位置を毎フレーム更新する
void BlockGrabber::UpdateCarriedPos(const Math::Vector3& playerPos, const Math::Matrix& playerRotMat)
{
	if (!m_spCarriedBlock) return;

	Math::Vector3 targetPos = CalcTargetPos(playerPos, playerRotMat);

	constexpr float minY = BlockGridManager::GridSize * 0.5f;

	// 「置いたらここに置かれる」位置(グリッドにスナップした位置)を計算
	Math::Vector3 snappedPreview = BlockGridManager::SnapToGrid(targetPos);
	if (snappedPreview.y < minY) snappedPreview.y = minY;

	m_previewPos = snappedPreview;
	// そのマスが空いているかどうかで、プレビューの色(緑/赤)を切り替える判定材料にする
	m_previewValid = !BlockGridManager::Instance().IsOccupied(snappedPreview);

	// 実際に持っているブロック本体は(グリッドにスナップしない)なめらかな位置に追従させる
	m_spCarriedBlock->SetPos(targetPos);
}

// マウスホイールで、ブロックを持つ距離(プレイヤーからどれだけ離すか)を調整する
void BlockGrabber::HandleDistanceControl()
{
	if (!m_spCarriedBlock) return;

	int wheelValue = Application::Instance().GetMouseWheelValue();

	if (wheelValue != 0)
	{
		// ホイールの回転量を正規化(WHEEL_DELTA=120が「1クリック分」)
		float scrollDelta = static_cast<float>(wheelValue) / 120.0f;

		// 上回しで遠く、下回しで近くへ
		m_holdDistance += scrollDelta * 2.5f;

		// 極端に遠すぎ/近すぎにならないよう範囲を制限
		m_holdDistance = std::clamp(m_holdDistance, 10.0f, 60.0f);
	}
}

// ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== =====
// 「今持っているブロックをどこに配置しようとしているか」の目標座標を計算する
// カメラの向き・保持距離・他のブロックとの重なり・プレイヤーとの最低距離、
// 全部を考慮した最終的な座標がここで決まる
// ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== =====
Math::Vector3 BlockGrabber::CalcTargetPos(const Math::Vector3& playerPos, const Math::Matrix& playerRotMat) const
{
	Math::Vector3 eyePos = playerPos;

	Math::Vector3 rawForward = playerRotMat.Backward(); // カメラが向いている方向(上下含む)

	// 上下方向(Y)を無視した「水平方向の向き」を作る(ブロックが変な高さに飛ばないようにするため)
	Math::Vector3 lookDirFlat = rawForward;
	lookDirFlat.y = 0.0f;
	lookDirFlat.Normalize();

	// 基本の目標位置：プレイヤーの水平な向きに、保持距離分だけ進んだ位置
	Math::Vector3 targetPos = eyePos + lookDirFlat * m_holdDistance;

	// 見上げ/見下ろしの角度に応じて、ブロックの高さも上下させる(上を見れば高い位置に置ける)
	float pitchFactor = rawForward.y;
	constexpr float verticalRange = 30.0f;
	float pitchOffset = pitchFactor * verticalRange;

	// 視線方向に壁などがないか、レイでチェックする
	// (壁の向こう側にブロックを出現させないための処理)
	KdCollider::RayInfo rayInfo;
	rayInfo.m_pos = eyePos;
	rayInfo.m_dir = lookDirFlat;
	rayInfo.m_range = m_holdDistance;
	rayInfo.m_type = KdCollider::TypeBump | KdCollider::TypeGround;

	std::list<KdCollider::CollisionResult> resultList;
	float closestDist = m_holdDistance;
	bool isHit = false;

	for (auto& obj : SceneManager::Instance().GetObjList())
	{
		if (obj == m_spCarriedBlock) continue; // 自分自身(持っているブロック)には当たらないようにする

		if (obj->Intersects(rayInfo, &resultList))
		{
			for (auto& ret : resultList)
			{
				float dist = (ret.m_hitPos - rayInfo.m_pos).Length();
				if (dist < closestDist)
				{
					closestDist = dist;
					isHit = true;
				}
			}
			resultList.clear();
		}
	}

	// 壁に当たっていたら、壁の手前(ブロック1個分くらい)までしか置けないよう距離を縮める
	if (isHit)
	{
		constexpr float blockRadius = BlockGridManager::GridSize;
		float adjustedDist = std::max(0.0f, closestDist - blockRadius);
		targetPos = eyePos + lookDirFlat * adjustedDist;
	}

	// 高さを、プレイヤーの目線 + 見上げ/見下ろし補正 で決定
	constexpr float baseHeightOffset = 0.0f;
	targetPos.y = eyePos.y + baseHeightOffset + pitchOffset;

	// ここまでで計算した「理想の目標位置」に対して、
	// 他のブロックに重ならないようスライド移動(SlideMove)で調整する
	Math::Vector3 oldPos = m_spCarriedBlock->GetPos();
	targetPos = SlideMove(oldPos, targetPos, m_spCarriedBlock);

	// プレイヤーに近づきすぎないよう最終調整
	ClampMinDistanceFromPlayer(targetPos, eyePos, lookDirFlat);

	// 最後に、地面より下に潜らないよう下限を保証
	constexpr float minY = BlockGridManager::GridSize * 0.5f;
	if (targetPos.y < minY) targetPos.y = minY;

	return targetPos;
}