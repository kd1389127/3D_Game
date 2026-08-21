#include "BlockGrabber.h"
#include "../../../Block/NormalBlock/NormalBlock.h"
#include "../../../Block/BlockGridManager.h"
#include "../../../../Scene/SceneManager.h"
#include "../../../../main.h"

namespace
{
	// 他の設置済みブロックとの重なりを、最小移動量で解消する
	void ResolveOverlapWithBlocks(Math::Vector3& pos, const std::shared_ptr<NormalBlock>& ignoreBlock)
	{
		constexpr float blockSize = BlockGridManager::GridSize; // 8.0f (ブロック1辺の長さ)

		for (auto& obj : SceneManager::Instance().GetObjList())
		{
			auto block = std::dynamic_pointer_cast<NormalBlock>(obj);
			if (!block || block == ignoreBlock || block->IsCarried()) continue;

			Math::Vector3 diff = pos - block->GetPos();

			float overlapX = blockSize - fabsf(diff.x);
			float overlapY = blockSize - fabsf(diff.y);
			float overlapZ = blockSize - fabsf(diff.z);

			// 3軸すべてで重なっている時だけ、実際に3Dとして重なっている
			if (overlapX > 0.0f && overlapY > 0.0f && overlapZ > 0.0f)
			{
				// 最も浅く重なっている軸を選び、その軸だけ押し出す
				if (overlapX <= overlapY && overlapX <= overlapZ)
				{
					pos.x += (diff.x >= 0.0f ? overlapX : -overlapX);
				}
				else if (overlapY <= overlapX && overlapY <= overlapZ)
				{
					pos.y += (diff.y >= 0.0f ? overlapY : -overlapY);
				}
				else
				{
					pos.z += (diff.z >= 0.0f ? overlapZ : -overlapZ);
				}
			}
		}
	}
	// ★ 追加：プレイヤー自身との最小距離を水平方向で保証する
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
				Math::Vector3 rawForward = playerRotMat.Backward(); // ピッチ込みの向き

				// 水平方向(ヨーのみ)
				Math::Vector3 lookDir = rawForward;
				lookDir.y = 0.0f;
				lookDir.Normalize();

				Math::Vector3 targetPos = playerPos + lookDir * m_holdDistance;

				// UpdateCarriedPos と同じ pitchOffset の計算をここにも適用
				float pitchFactor = rawForward.y;
				constexpr float verticalRange = 30.0f; // UpdateCarriedPos と必ず同じ値にする
				float pitchOffset = pitchFactor * verticalRange;

				constexpr float baseHeightOffset = 0.0f; // UpdateCarriedPos と必ず同じ値にする
				targetPos.y = playerPos.y + baseHeightOffset + pitchOffset;

				// 重なり解消
				ResolveOverlapWithBlocks(targetPos, m_spCarriedBlock);

				constexpr float minY = BlockGridManager::GridSize * 0.5f;
				if (targetPos.y < minY) targetPos.y = minY;
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

	Math::Vector3 eyePos = playerPos;

	Math::Vector3 rawForward = playerRotMat.Backward(); // ピッチ込みの向き

	// 水平方向(ヨーのみ)：前後・左右の位置決めに使う
	Math::Vector3 lookDirFlat = rawForward;
	lookDirFlat.y = 0.0f;
	lookDirFlat.Normalize();

	Math::Vector3 targetPos = eyePos + lookDirFlat * m_holdDistance;

	// ピッチ成分(-1〜1程度)を高さの追加オフセットとして使う
	//  rawForward.y は見上げると+方向、見下げると-方向に近い値になる
	float pitchFactor = rawForward.y; // 正規化済みなのでおおよそ-1〜1
	constexpr float verticalRange = 30.0f; // 上下に動かせる最大幅(お好みで調整)
	float pitchOffset = pitchFactor * verticalRange;

	// レイ判定は水平方向のまま(前後の壁チェック用)
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

	// 高さ = 基準の高さ + ピッチによる可変オフセット
	constexpr float baseHeightOffset = 0.0f; // 基準の高さ(目線あたり)
	targetPos.y = eyePos.y + baseHeightOffset + pitchOffset;

	// 他の設置済みブロックとの重なりを解消(ピタッと張り付く)
	ResolveOverlapWithBlocks(targetPos, m_spCarriedBlock);

	// プレイヤーとの最小距離を保証
	ClampMinDistanceFromPlayer(targetPos, eyePos, lookDirFlat);

	// 最低高度の制限（ここは今まで通り重要）
	constexpr float minY = BlockGridManager::GridSize * 0.5f;
	if (targetPos.y < minY)
	{
		targetPos.y = minY;
	}

	m_spCarriedBlock->SetPos(targetPos);
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
