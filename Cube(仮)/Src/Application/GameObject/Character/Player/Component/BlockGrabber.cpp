#include "BlockGrabber.h"
#include "../../../Block/NormalBlock/NormalBlock.h"
#include "../../../Block/BlockGridManager.h"
#include "../../../../Scene/SceneManager.h"
#include "../../../../main.h"

namespace
{
	bool OverlapsAnyBlock(const Math::Vector3& pos, const std::shared_ptr<NormalBlock>& ignoreBlock)
	{
		constexpr float blockSize = BlockGridManager::GridSize; // 8.0f

		for (auto& obj : SceneManager::Instance().GetObjList())
		{
			auto block = std::dynamic_pointer_cast<NormalBlock>(obj);
			if (!block || block == ignoreBlock || block->IsCarried()) continue;

			Math::Vector3 diff = pos - block->GetPos();
			if (fabsf(diff.x) < blockSize && fabsf(diff.y) < blockSize && fabsf(diff.z) < blockSize)
				return true;
		}
		return false;
	}

	// oldPos(重なっていない安全な位置)から desiredPos へ、軸ごとに「動けるところまで」動かす
	Math::Vector3 SlideMove(const Math::Vector3& oldPos, const Math::Vector3& desiredPos,
		const std::shared_ptr<NormalBlock>& ignoreBlock)
	{
		Math::Vector3 result = oldPos;

		Math::Vector3 tryX = result; tryX.x = desiredPos.x;
		if (!OverlapsAnyBlock(tryX, ignoreBlock)) result.x = desiredPos.x;

		Math::Vector3 tryZ = result; tryZ.z = desiredPos.z;
		if (!OverlapsAnyBlock(tryZ, ignoreBlock)) result.z = desiredPos.z;

		Math::Vector3 tryY = result; tryY.y = desiredPos.y;
		if (!OverlapsAnyBlock(tryY, ignoreBlock)) result.y = desiredPos.y;

		return result;
	}
	// プレイヤー自身との最小距離を水平方向で保証する
	void ClampMinDistanceFromPlayer(Math::Vector3& pos, const Math::Vector3& playerPos, const Math::Vector3& fallbackDir)
	{
		constexpr float minPlayerDist = BlockGridManager::GridSize; // 8.0f (必要ならプレイヤーの当たり判定サイズに合わせて調整)

		Math::Vector3 toBlock = pos - playerPos;
		toBlock.y = 0.0f; // 水平方向だけで判定

		float dist = toBlock.Length();

		if (dist < minPlayerDist)
		{
			Math::Vector3 dir;
			if (dist > 0.0001f)
			{
				dir = toBlock / dist; // 正規化
			}
			else
			{
				dir = fallbackDir; // ほぼ真上/真下などで方向が定まらない場合の保険
			}

			pos.x = playerPos.x + dir.x * minPlayerDist;
			pos.z = playerPos.z + dir.z * minPlayerDist;
		}
	}
}

void BlockGrabber::Init()
{
	m_spPreviewModel = std::make_shared<KdModelWork>();
	m_spPreviewModel->SetModelData("Asset/Models/Block/WoodenBox/Wooden_Box.gltf"); // NormalBlockと同じパス

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

void BlockGrabber::Update(const Math::Vector3& playerPos, const Math::Matrix& playerRotMat)
{
	HandleGrabAndDrop(playerPos, playerRotMat);
	UpdateCarriedPos(playerPos, playerRotMat);
	HandleDistanceControl(); // ホイール処理
}

Math::Vector3 BlockGrabber::GetCarriedBlockPos() const
{
	return m_spCarriedBlock ? m_spCarriedBlock->GetPos() : Math::Vector3::Zero;
}

void BlockGrabber::DrawPreview()
{
	if (!m_spCarriedBlock || !m_spPreviewModel) return;

	auto context = KdDirect3D::Instance().WorkDevContext();

	// 現在のブレンドステートを退避
	Microsoft::WRL::ComPtr<ID3D11BlendState> prevBlendState;
	float prevBlendFactor[4];
	UINT prevSampleMask;
	context->OMGetBlendState(prevBlendState.GetAddressOf(), prevBlendFactor, &prevSampleMask);

	// 半透明用に切り替え
	float blendFactor[4] = { 1,1,1,1 };
	context->OMSetBlendState(m_alphaBlendState.Get(), blendFactor, 0xFFFFFFFF);

	Math::Matrix mat = Math::Matrix::CreateScale(8.0f) * Math::Matrix::CreateTranslation(m_previewPos);

	// ★ 置けるかどうかで色を変える
	Math::Color previewColor = m_previewValid
		? Math::Color(0.5f, 1.0f, 0.5f, 0.6f)  // 緑・半透明
		: Math::Color(1.0f, 0.3f, 0.3f, 0.6f); // 赤・半透明

	KdShaderManager::Instance().m_StandardShader.DrawModel(*m_spPreviewModel, mat, previewColor);

	// 元のブレンドステートに戻す
	context->OMSetBlendState(prevBlendState.Get(), prevBlendFactor, prevSampleMask);
}

void BlockGrabber::HandleGrabAndDrop(const Math::Vector3& playerPos, const Math::Matrix& playerRotMat)
{
	if (GetAsyncKeyState('E') & 0x8000)
	{
		if (!m_eKeyFlg)
		{
			m_eKeyFlg = true;

			// 置く処理
			if (m_spCarriedBlock)
			{
				Math::Vector3 targetPos = CalcTargetPos(playerPos, playerRotMat); // ★ 同じ関数を使う

				constexpr float minY = BlockGridManager::GridSize * 0.5f;
				Math::Vector3 snappedPos = BlockGridManager::SnapToGrid(targetPos);
				if (snappedPos.y < minY) snappedPos.y = minY;

				if (!BlockGridManager::Instance().IsOccupied(snappedPos))
				{
					m_spCarriedBlock->SetPos(snappedPos);
					m_spCarriedBlock->SetCarried(false);
					BlockGridManager::Instance().Register(snappedPos);
					m_spCarriedBlock = nullptr;
				}
			}
			// 掴む処理
			else
			{
				KdCollider::RayInfo rayInfo;
				rayInfo.m_pos = playerPos;
				rayInfo.m_dir = playerRotMat.Backward();
				rayInfo.m_range = 30.0f;
				rayInfo.m_type = KdCollider::TypeBump | KdCollider::TypeGround;

				std::list<KdCollider::CollisionResult> resultList;
				std::shared_ptr<NormalBlock> targetBlock = nullptr;
				float minDist = rayInfo.m_range;

				for (auto& obj : SceneManager::Instance().GetObjList())
				{
					auto block = std::dynamic_pointer_cast<NormalBlock>(obj);
					if (!block || block->IsCarried()) continue;

					if (block->Intersects(rayInfo, &resultList))
					{
						for (auto& ret : resultList)
						{
							float dist = (ret.m_hitPos - rayInfo.m_pos).Length();
							if (dist < minDist)
							{
								minDist = dist;
								targetBlock = block;
							}
						}
						resultList.clear();
					}
				}

				if (targetBlock)
				{
					m_spCarriedBlock = targetBlock;
					m_holdDistance = 30.0f;
					Math::Vector3 snappedPos = BlockGridManager::SnapToGrid(targetBlock->GetPos());
					BlockGridManager::Instance().Unregister(snappedPos);
					m_spCarriedBlock->SetCarried(true);
				}
			}
		}
	}
	else
	{
		m_eKeyFlg = false;
	}
}

void BlockGrabber::UpdateCarriedPos(const Math::Vector3& playerPos, const Math::Matrix& playerRotMat)
{
	if (!m_spCarriedBlock) return;

	Math::Vector3 targetPos = CalcTargetPos(playerPos, playerRotMat);

	constexpr float minY = BlockGridManager::GridSize * 0.5f;

	Math::Vector3 snappedPreview = BlockGridManager::SnapToGrid(targetPos);
	if (snappedPreview.y < minY) snappedPreview.y = minY;

	m_previewPos = snappedPreview;
	m_previewValid = !BlockGridManager::Instance().IsOccupied(snappedPreview);

	m_spCarriedBlock->SetPos(targetPos);
	//m_spCarriedBlock->SetPos(snappedPreview);
}

void BlockGrabber::HandleDistanceControl()
{
	if (!m_spCarriedBlock) return;

	// Application からホイールの回転量を取得
	int wheelValue = Application::Instance().GetMouseWheelValue();

	if (wheelValue != 0)
	{
		// ホイール値を 120 (WHEEL_DELTA) で割って正規化
		float scrollDelta = static_cast<float>(wheelValue) / 120.0f;

		// 上回しで遠く(奥)、下回しで近く(手前)へ移動
		m_holdDistance += scrollDelta * 2.5f; // 2.5f は移動スピードの設定

		// 距離の制限（例: 10.0f ～ 60.0f の範囲に収める）
		m_holdDistance = std::clamp(m_holdDistance, 10.0f, 60.0f);
	}
}

Math::Vector3 BlockGrabber::CalcTargetPos(const Math::Vector3& playerPos, const Math::Matrix& playerRotMat) const
{
	Math::Vector3 eyePos = playerPos;

	Math::Vector3 rawForward = playerRotMat.Backward();

	Math::Vector3 lookDirFlat = rawForward;
	lookDirFlat.y = 0.0f;
	lookDirFlat.Normalize();

	Math::Vector3 targetPos = eyePos + lookDirFlat * m_holdDistance;

	float pitchFactor = rawForward.y;
	constexpr float verticalRange = 30.0f;
	float pitchOffset = pitchFactor * verticalRange;

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
		if (obj == m_spCarriedBlock) continue;

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

	if (isHit)
	{
		constexpr float blockRadius = BlockGridManager::GridSize;
		float adjustedDist = std::max(0.0f, closestDist - blockRadius);
		targetPos = eyePos + lookDirFlat * adjustedDist;
	}

	constexpr float baseHeightOffset = 0.0f;
	targetPos.y = eyePos.y + baseHeightOffset + pitchOffset;

	Math::Vector3 oldPos = m_spCarriedBlock->GetPos();
	targetPos = SlideMove(oldPos, targetPos, m_spCarriedBlock);

	ClampMinDistanceFromPlayer(targetPos, eyePos, lookDirFlat);

	constexpr float minY = BlockGridManager::GridSize * 0.5f;
	if (targetPos.y < minY) targetPos.y = minY;

	return targetPos;
}
