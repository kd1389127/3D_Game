#include "BlockGrabber.h"
#include "../../../Block/NormalBlock/NormalBlock.h"
#include "../../../Block/BlockGridManager.h"
#include "../../../../Scene/SceneManager.h"
#include "../../../../main.h"

namespace
{
	// 指定した座標にブロック(8x8x8)を置いた時、グリッド上のブロック(Map・魔法ブロック)にめり込むか
	// ブロックの8つの角が、埋まっているマスの中に入っていないかを調べる
	bool OverlapsGridBlock(const Math::Vector3& pos)
	{
		constexpr float inset = BlockGridManager::GridSize * 0.5f - 0.01f; // 角ぎりぎり(3.99)

		for (int sx : { -1, 1 })
		{
			for (int sy : { -1, 1 })
			{
				for (int sz : { -1, 1 })
				{
					Math::Vector3 p = pos + Math::Vector3(sx * inset, sy * inset, sz * inset);
					Math::Vector3 cell = BlockGridManager::Instance().SnapToGrid(p);
					if (BlockGridManager::Instance().IsOccupied(cell)) return true;
				}
			}
		}
		return false;
	}

	// 指定位置のブロックが、壁・階段・檻などのコライダーに重なっているか
	bool OverlapsColliders(const Math::Vector3& pos, const KdGameObject* self)
	{
		constexpr float half = BlockGridManager::GridSize * 0.5f - 0.05f; // ぴったり接するだけなら重なり扱いにしない

		DirectX::BoundingBox box;
		box.Center = pos;
		box.Extents = DirectX::XMFLOAT3(half, half, half);
		KdCollider::BoxInfo info(KdCollider::TypeBump, box);

		for (auto& obj : SceneManager::Instance().GetObjList())
		{
			if (obj.get() == self) continue;
			if (std::dynamic_pointer_cast<IGrabbable>(obj)) continue; // ブロック同士は別の関数で見る

			std::list<KdCollider::CollisionResult> results;
			if (obj->Intersects(info, &results)) return true;
		}
		return false;
	}

	// 指定した座標(pos)に置くと、何かに重なるかチェックする
	// ① グリッドに登録されたブロック ② 他のギミックブロック ③ 壁・階段・檻のコライダー
	bool OverlapsAnyBlock(const Math::Vector3& pos, const std::shared_ptr<KdGameObject>& ignoreBlock)
	{
		if (OverlapsGridBlock(pos)) return true;

		constexpr float blockSize = BlockGridManager::GridSize;

		for (auto& obj : SceneManager::Instance().GetObjList())
		{
			auto grabbable = std::dynamic_pointer_cast<IGrabbable>(obj);
			if (!grabbable || obj == ignoreBlock || grabbable->IsCarried()) continue;

			Math::Vector3 diff = pos - obj->GetPos();
			if (fabsf(diff.x) < blockSize && fabsf(diff.y) < blockSize && fabsf(diff.z) < blockSize)
				return true;
		}

		return OverlapsColliders(pos, ignoreBlock.get());
	}

	// 現在位置から、1つの軸だけ目標値へ少しずつ動かし、何かに重なる直前で止める
	// (一気に飛ぶと、格子や薄い壁をすり抜けてしまうため)
	Math::Vector3 SweepAxis(const Math::Vector3& from, int axis, float target,
		const std::shared_ptr<KdGameObject>& ignoreBlock)
	{
		constexpr float stepSize = 2.0f;

		const float start = (&from.x)[axis];
		const float diff = target - start;
		const int steps = std::max(1, (int)std::ceil(fabsf(diff) / stepSize));

		Math::Vector3 result = from;
		for (int i = 1; i <= steps; ++i)
		{
			Math::Vector3 p = from;
			(&p.x)[axis] = start + diff * ((float)i / steps);

			if (OverlapsAnyBlock(p, ignoreBlock)) break; // ここから先は重なるので、手前で止める
			result = p;
		}
		return result;
	}

	// 持っているブロックを「安全な位置(oldPos)」から「目標位置(desiredPos)」へ移動させる
	// X→Z→Yの順に1軸ずつ動かすので、壁際を滑るように動ける
	Math::Vector3 SlideMove(const Math::Vector3& oldPos, const Math::Vector3& desiredPos,
		const std::shared_ptr<KdGameObject>& ignoreBlock)
	{
		// 今の位置がすでに埋まっている時は動きを制限しない(後のPushOutFromCollidersで押し出される)
		// 制限すると、埋まったまま動かせず、離せもしない状態になるため
		if (OverlapsAnyBlock(oldPos, ignoreBlock)) return desiredPos;

		Math::Vector3 result = oldPos;
		result = SweepAxis(result, 0, desiredPos.x, ignoreBlock); // X
		result = SweepAxis(result, 2, desiredPos.z, ignoreBlock); // Z
		result = SweepAxis(result, 1, desiredPos.y, ignoreBlock); // Y
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

	// 持っているブロックが、壁・階段・檻などのコライダーにめり込んでいたら押し戻す
	// グリッドに登録されていないもの(檻・外枠など)や、斜めの階段にも効く最後の安全網
	Math::Vector3 PushOutFromColliders(Math::Vector3 pos, const KdGameObject* self)
	{
		// 少し小さめの箱で調べる(壁にぴったり接しているだけでは、当たり扱いにしないため)
		constexpr float half = BlockGridManager::GridSize * 0.5f - 0.05f;

		DirectX::BoundingBox box;
		box.Extents = DirectX::XMFLOAT3(half, half, half);
		KdCollider::BoxInfo info(KdCollider::TypeBump, box);

		// 押し戻した先で別の壁にめり込むことがあるので、最大3回まで繰り返す
		for (int iter = 0; iter < 3; ++iter)
		{
			bool pushed = false;

			for (auto& obj : SceneManager::Instance().GetObjList())
			{
				if (obj.get() == self) continue;                         // 自分自身
				if (std::dynamic_pointer_cast<IGrabbable>(obj)) continue; // 他のギミックブロックは対象外

				info.m_Abox.Center = pos;

				std::list<KdCollider::CollisionResult> results;
				if (!obj->Intersects(info, &results)) continue;

				for (auto& ret : results)
				{
					if (ret.m_overlapDistance <= 0.0f) continue;

					// 重なった分だけ、壁から離れる向きに動かす
					pos += ret.m_hitDir * ret.m_overlapDistance;
					pushed = true;
				}
			}

			if (!pushed) break;
		}
		return pos;
	}
}

// 初期化：プレビュー用モデルの読み込みと、半透明描画のための設定(ブレンドステート)を準備
void BlockGrabber::Init(float groundHeight)
{
	m_groundHeight = groundHeight;
}

// 毎フレームの更新：「掴む/離す」「持っているブロックの位置更新」「ホイールでの距離調整」をまとめて呼ぶ
void BlockGrabber::Update(const Math::Vector3& playerPos, const Math::Matrix& playerRotMat)
{
	HandleGrabAndDrop(playerPos, playerRotMat);
	UpdateCarriedPos(playerPos, playerRotMat);
	UpdateGrabTarget(playerPos, playerRotMat);
}

float BlockGrabber::GetMinY() const
{
	return m_groundHeight + BlockGridManager::GridSize * 0.5f;
}

// 今持っているブロックの座標を返す(何も持っていなければ原点を返す)
Math::Vector3 BlockGrabber::GetCarriedBlockPos() const
{
	return m_spCarriedBlock ? m_spCarriedBlock->GetPos() : Math::Vector3::Zero;
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
			// 位置はその場のまま。グリッドには固定せず、離した後はブロック側の重力で落ちる
			if (m_spCarriedBlock)
			{
				// めり込んだまま離すと、落下処理が押し上げて壁の上に出てしまうので、
				// 重なっている間は離せないようにする
				if (OverlapsAnyBlock(m_spCarriedBlock->GetPos(), m_spCarriedBlock)) return;

				auto grabbable = std::dynamic_pointer_cast<IGrabbable>(m_spCarriedBlock);
				if (grabbable) grabbable->SetCarried(false);
				m_spCarriedBlock = nullptr;
			}
			// ---- 何も持っていない場合 → 「掴む」処理 ----
			else
			{
				// 視線の先にあるギミックブロックを探す(魔法ブロックや壁が手前にあれば見つからない)
				std::shared_ptr<KdGameObject> targetBlock = FindGradTarget(playerPos, playerRotMat);

				// 見つかったブロックを「持っている状態」にする
				if (targetBlock)
				{
					m_spCarriedBlock = targetBlock;
					m_holdDistance = 30.0f;

					auto grabbable = std::dynamic_pointer_cast<IGrabbable>(m_spCarriedBlock);
					if (grabbable) grabbable->SetCarried(true);
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

// 持っているブロックの位置を毎フレーム更新する
void BlockGrabber::UpdateCarriedPos(const Math::Vector3& playerPos, const Math::Matrix& playerRotMat)
{
	if (!m_spCarriedBlock) return;

	m_spCarriedBlock->SetPos(CalcTargetPos(playerPos, playerRotMat));
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

	// 見上げ/見下ろしの角度に応じて、ブロックの高さも上下させる
	float pitchFactor = rawForward.y;
	constexpr float verticalRange = 30.0f;
	float pitchOffset = pitchFactor * verticalRange;

	// 視線方向に壁などがないか、レイでチェックする(壁の向こう側にブロックを出現させないための処理)
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

	// 高さ：プレイヤーの目線 + 見上げ/見下ろし補正
	targetPos.y = eyePos.y + pitchOffset;

	// プレイヤーに近づきすぎないよう、先に目標位置を調整する
	ClampMinDistanceFromPlayer(targetPos, eyePos, lookDirFlat);

	// 地面より下に潜らないよう下限を保証
	const float minY = GetMinY();
	if (targetPos.y < minY) targetPos.y = minY;

	// 他のブロックや壁・床に重ならないようスライド移動で調整する
	Math::Vector3 oldPos = m_spCarriedBlock->GetPos();
	targetPos = SlideMove(oldPos, targetPos, m_spCarriedBlock);

	// 最後の安全網：グリッド外の壁・檻・階段にめり込んでいたら押し戻す
	targetPos = PushOutFromColliders(targetPos, m_spCarriedBlock.get());
	if (targetPos.y < minY) targetPos.y = minY;

	return targetPos;
}

// 視線の先で、一番手前に当たったものが「ギミックブロック」のときだけ、それを返す
std::shared_ptr<KdGameObject> BlockGrabber::FindGradTarget(const Math::Vector3& playerPos, const Math::Matrix& playerRotMat) const
{
	KdCollider::RayInfo rayInfo;
	rayInfo.m_pos = playerPos;
	rayInfo.m_dir = playerRotMat.Backward();
	rayInfo.m_range = 30.0f;
	rayInfo.m_type = KdCollider::TypeBump | KdCollider::TypeGround;

	float minDist = rayInfo.m_range;                        // 何であれ一番手前のもの
	std::shared_ptr<KdGameObject> gimmickObj = nullptr;     // 一番手前のギミックブロック
	float gimmickDist = rayInfo.m_range;

	// 壁も含めて、レイが最初に当たったものを探す
	for (auto& obj : SceneManager::Instance().GetObjList())
	{
		auto grabbable = std::dynamic_pointer_cast<IGrabbable>(obj);
		if (grabbable && grabbable->IsCarried()) continue;

		std::list<KdCollider::CollisionResult> resultList;
		if (!obj->Intersects(rayInfo, &resultList)) continue;

		bool isGimmick = grabbable && grabbable->GetBlockKind() == BlockGridManager::BlockKind::GimmickKey;

		for (auto& ret : resultList)
		{
			float dist = (ret.m_hitPos - rayInfo.m_pos).Length();
			if (dist < minDist) minDist = dist;
			if (isGimmick && dist < gimmickDist)
			{
				gimmickDist = dist;
				gimmickObj = obj;
			}
		}
	}

	// ギミックブロックが、一番手前のものから1マス以内にあれば掴める
	// (檻などにめり込んだブロックを、取り出せるようにするため)
	if (gimmickObj && gimmickDist <= minDist + BlockGridManager::GridSize) return gimmickObj;

	return nullptr;
}

// 毎フレーム、「今、掴めるギミックブロックを狙っているか」を更新する
void BlockGrabber::UpdateGrabTarget(const Math::Vector3& playerPos, const Math::Matrix& playerRotMat)
{
	// 何かを持っている間は、探す必要がない
	if (m_spCarriedBlock)
	{
		m_hasGrabTarget = false;
		SetGrabHighlightTarget(nullptr);
		return;
	}

	auto target = FindGradTarget(playerPos, playerRotMat);
	m_hasGrabTarget = (target != nullptr);
	SetGrabHighlightTarget(target);
}

void BlockGrabber::SetGrabHighlightTarget(const std::shared_ptr<KdGameObject>& target)
{
	auto prev = m_wpHighlighted.lock();
	if (prev == target) return;	// 対象が変わっていなければ何もしない

	if (prev)
	{
		if (auto grabbable = std::dynamic_pointer_cast<IGrabbable>(prev))
		{
			grabbable->SetGrabHighlight(false);
		}
	}

	if (target)
	{
		if (auto grabbable = std::dynamic_pointer_cast<IGrabbable>(target))
		{
			grabbable->SetGrabHighlight(true);
		}
	}

	m_wpHighlighted = target;

}