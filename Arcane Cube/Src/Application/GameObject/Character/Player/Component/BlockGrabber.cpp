#include "BlockGrabber.h"
#include "../../../Block/NormalBlock/NormalBlock.h"
#include "../../../Block/BlockGridManager.h"
#include "../../../Map/MapBlock/MapBlock.h"
#include "../../../Block/GimmickBlock/GimmickBlock.h"
#include "../../../../Scene/SceneManager.h"
#include "../../../Collision/Collision.h"

namespace
{
	constexpr float kGrabRange		= 30.0f; // 掴める距離
	constexpr float kGrabSenseRange = 45.0f; //「遠い」と教えるために探す距離
	constexpr float kSweepStep      = 2.0f;  // 1回に動かす距離
	constexpr float kCullMargin     = 16.0f; // MapBlockを近くに絞る時の余裕
	constexpr float kGrabEffectSize = 6.0f;  // 掴み中エフェクトの大きさ

	using ColliderList = std::vector<KdGameObject*>;

	// 判定の相手(壁・檻・階段など)の一覧を作る。1フレームに1回だけ呼んで、あとは使い回す
	//  ・自分自身とギミックブロックは入れない(ブロック同士は IsOverlappingFreeBlock で見る)
	//  ・MapBlockは約650個あるので、動く範囲(from→to)の近くにあるものだけ入れる
	ColliderList BuildColliderList(const KdGameObject* self, const Math::Vector3& from, const Math::Vector3& to)
	{
		const Math::Vector3 center = (from + to) * 0.5f;				// 動く線の真ん中
		const float radius = (to - from).Length() * 0.5f + kCullMargin;	// 線の半分の長さ+余裕
		const float radiusSq = radius * radius;

		ColliderList list;
		for (auto& obj : SceneManager::Instance().GetObjList())
		{
			if (obj.get() == self) continue;
			if (dynamic_cast<IGrabbable*>(obj.get()))continue;

			if (dynamic_cast<MapBlock*>(obj.get()))
			{
				if (Math::Vector3::DistanceSquared(obj->GetPos(), center) > radiusSq) continue;
			}
			list.push_back(obj.get());
		}
		return list;
	}

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
	bool OverlapsColliders(const Math::Vector3& pos, const ColliderList& colliders)
	{
		constexpr float half = BlockGridManager::GridSize * 0.5f - 0.05f; // ぴったり接するだけなら重なり扱いにしない

		DirectX::BoundingBox box;
		box.Center = pos;
		box.Extents = DirectX::XMFLOAT3(half, half, half);
		KdCollider::BoxInfo info(KdCollider::TypeBump, box);

		// 全オブジェクトではなく、作っておいた一覧だけを見る
		for (KdGameObject* obj : colliders)
		{
			// 重なっているかだけ知りたいので結果リストは不要
			if (obj->Intersects(info, nullptr)) return true;
		}
		return false;
	}

	// ① グリッド上のブロック ② 他のギミックブロック ③ 壁・階段・檻のコライダー
	bool OverlapsAnyBlock(const Math::Vector3& pos, const KdGameObject* self, const ColliderList& colliders)
	{
		if (OverlapsGridBlock(pos)) return true;
		if (BlockGridManager::Instance().IsOverlappingFreeBlock(pos, self)) return true;
		return OverlapsColliders(pos, colliders);
	}

	// 現在位置から、1つの軸だけ目標値へ少しずつ動かし、何かに重なる直前で止める
	// (一気に飛ぶと、格子や薄い壁をすり抜けてしまうため)
	Math::Vector3 SweepAxis(const Math::Vector3& from, int axis, float target,
		const KdGameObject* self, const ColliderList& colliders)
	{
		const float start = (&from.x)[axis];
		const float diff = target - start;
		const int steps = std::max(1, (int)std::ceil(fabsf(diff) / kSweepStep));

		Math::Vector3 result = from;
		for (int i = 1; i <= steps; ++i)
		{
			Math::Vector3 p = from;
			(&p.x)[axis] = start + diff * ((float)i / steps);

			if (OverlapsAnyBlock(p, self, colliders)) break; // ここから先は重なるので、手前で止める
			result = p;
		}
		return result;
	}

	// 持っているブロックを「安全な位置(oldPos)」から「目標位置(desiredPos)」へ移動させる
	// X→Z→Yの順に1軸ずつ動かすので、壁際を滑るように動ける
	Math::Vector3 SlideMove(const Math::Vector3& oldPos, const Math::Vector3& desiredPos,
		const KdGameObject* self, const ColliderList& colliders)
	{
		// すでに埋まっている時：目標へ丸ごと動かすと壁の向こう側へ飛べてしまうので、
		// 1フレームに動ける距離だけ制限する(止めると埋まったまま動かせなくなる)
		if (OverlapsAnyBlock(oldPos, self, colliders))
		{
			Math::Vector3 move = desiredPos - oldPos;
			const float len = move.Length();
			if (len > kSweepStep) move *= kSweepStep / len;
			return oldPos + move;
		}

		Math::Vector3 result = oldPos;
		result = SweepAxis(result, 0, desiredPos.x, self, colliders); // X
		result = SweepAxis(result, 2, desiredPos.z, self, colliders); // Z
		result = SweepAxis(result, 1, desiredPos.y, self, colliders); // Y
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
	Math::Vector3 PushOutFromColliders(Math::Vector3 pos, const ColliderList& colliders)	// 引数を変更
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

			for (KdGameObject* obj : colliders)
			{
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

	struct GrabCandidate
	{
		std::shared_ptr<KdGameObject> obj;	// 視線の先に見えているギミックブロック(無ければnullptr)
		float dist = 0.0f;					// 目からその距離
	};

	// 遠くても(kGrabSenseRangeまで)見つけて返す。「届くか」は呼び出し側が距離で決める
	GrabCandidate FindGrabCandidate(const Math::Vector3& playerPos, const Math::Matrix& playerRotMat)
	{
		KdCollider::RayInfo rayInfo;
		rayInfo.m_pos = playerPos;
		rayInfo.m_dir = playerRotMat.Backward();
		rayInfo.m_range = kGrabSenseRange;
		rayInfo.m_type = KdCollider::TypeBump | KdCollider::TypeGround;

		std::vector<KdGameObject*> candidates;
		CollisionFilter::CollectAlongRay(rayInfo.m_pos, rayInfo.m_dir, rayInfo.m_range, candidates);

		float minDist = rayInfo.m_range;		// 何であれ一番手前に当たったものまでの距離
		float gimmickDist = rayInfo.m_range;	// 一番手前のギミックブロックまでの距離
		GrabCandidate candidate;

		std::list<KdCollider::CollisionResult> resultList;
		for(KdGameObject* obj : candidates)
		{
			resultList.clear();
			if (!obj->Intersects(rayInfo, &resultList)) continue;

			auto grabbable = dynamic_cast<IGrabbable*>(obj);
			if (grabbable && grabbable->IsCarried()) continue;

			const bool isGimmick = grabbable &&
				grabbable->GetBlockKind() == BlockGridManager::BlockKind::GimmickKey;

			for (auto& ret : resultList)
			{
				const float dist = (ret.m_hitPos - rayInfo.m_pos).Length();
				if (dist < minDist) minDist = dist;
				if (isGimmick && dist < gimmickDist)
				{
					gimmickDist = dist;
					candidate.obj = obj->shared_from_this();	// 生ポインタ→shared_ptrへ
				}
			}
		}

		// 手前に壁があれば壁越しには狙えない(ほぼ同じ距離=0.5以内の時だけ有効)
		if (candidate.obj && gimmickDist <= minDist + 0.5f)
		{
			candidate.dist = gimmickDist;
			return candidate;
		}
		return GrabCandidate{};
	}

}

BlockGrabber::~BlockGrabber()
{
	StopGrabEffect();	// 持ったままゴールに入っても、エフェクトが次のシーンに残らないように
}

void BlockGrabber::Init(float groundHeight)
{
	m_groundHeight = groundHeight;		// ブロックが地面に潜らない下限(GetMinY)に使う
}

void BlockGrabber::Update(const Math::Vector3& playerPos, const Math::Matrix& playerRotMat)
{
	HandleGrabAndDrop(playerPos, playerRotMat);
	UpdateCarriedPos(playerPos, playerRotMat);
	UpdateGrabEffect();	
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

void BlockGrabber::ReleaseAndReturn()
{
	if (!m_spCarriedBlock) return;	// 何も持っていなければ、やることはない

	// 持っている状態を解除する(当たり判定も、ここで復活する)
	if (auto grabbable = std::dynamic_pointer_cast<IGrabbable>(m_spCarriedBlock))
	{
		grabbable->SetCarried(false);
	}

	// 元の場所に戻す(判定を復活させた後に動かす)
	if (auto gimmick = std::dynamic_pointer_cast<GimmickBlock>(m_spCarriedBlock))
	{
		gimmick->ResetToStartPos();
	}

	// こちらの持っている状態を全部クリアする
	SetGrabHighlightTarget(nullptr);	// 緑の点滅を消す
	StopGrabEffect();					// 掴み中のエフェクトを止める
	m_spCarriedBlock = nullptr;			// 持っている変数を空に
	m_hasGrabTarget  = false;			// 狙っている状態も戻す
	m_isTargetFar    = false;
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
				const KdGameObject* self = m_spCarriedBlock.get();
				const Math::Vector3 pos = m_spCarriedBlock->GetPos();
				const ColliderList colliders = BuildColliderList(self, pos, pos);	// 動かさないので from=to

				// めり込んだまま離すと落下処理が押し上げるので、重なっている間は離せない
				if (OverlapsAnyBlock(pos, self, colliders))
				{
					++m_dropBlockedId;	// 「離せなかった」ことをレティクルに知らせる
					return;
				}

				auto grabbable = std::dynamic_pointer_cast<IGrabbable>(m_spCarriedBlock);
				if (grabbable) grabbable->SetCarried(false);
				m_spCarriedBlock = nullptr;
				StopGrabEffect();
			}
			// ---- 何も持っていない場合 → 「掴む」処理 ----
			else
			{
				const GrabCandidate candidate = FindGrabCandidate(playerPos, playerRotMat);

				// 届く距離にある時だけ掴める
				if (candidate.obj && candidate.dist <= kGrabRange)
				{
					m_spCarriedBlock = candidate.obj;
					m_holdDistance = kGrabRange;

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

	std::vector<KdGameObject*> candidates;
	CollisionFilter::CollectAlongRay(rayInfo.m_pos, rayInfo.m_dir, rayInfo.m_range, candidates,
		m_spCarriedBlock.get());	// 持っているブロック自身は最初から除く

	for (KdGameObject* obj : candidates)
	{
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

	// ---- そこへ安全に動かす ----
	const KdGameObject* self = m_spCarriedBlock.get();
	const Math::Vector3 oldPos = m_spCarriedBlock->GetPos();
	const ColliderList colliders = BuildColliderList(self, oldPos, targetPos);	// 以降の判定で使い回す

	targetPos = SlideMove(oldPos, targetPos, self, colliders);

	// ---- 保険 ----
	targetPos = PushOutFromColliders(targetPos, colliders);
	if (targetPos.y < minY) targetPos.y = minY;

	// 最終確認：ここまでやっても重なっているなら、前フレームの安全な位置のままにする
	// (押し戻しで、薄い壁の向こう側に出てしまうのを防ぐ)
	if (OverlapsAnyBlock(targetPos, self, colliders) && !OverlapsAnyBlock(oldPos, self, colliders))
	{
		targetPos = oldPos;
	}

	// 壁沿いに滑った結果、プレイヤーに近づきすぎていたら、前の位置のままにする
	auto distSq = [&](const Math::Vector3& p)
	{
		const float dx = p.x - eyePos.x;
		const float dz = p.z - eyePos.z;
		return dx * dx + dz * dz;
	};
	constexpr float minDist = BlockGridManager::GridSize;	// ClampMinDistanceFromPlayerと同じ距離

	// 頭の上にあるブロックが、視点を下げた時にプレイヤーの体の高さへ下りてこないようにする
	if (distSq(targetPos) < minDist * minDist) // 横方向でプレイヤーの真上・真下あたりにいる時だけ
	{
		constexpr float blockHalf    = BlockGridManager::GridSize * 0.5f;
		constexpr float playerHeight = 10.0f;  // Player::m_adjustHeight と同じ値(目線から足元まで)

		// ブロックが、プレイヤーの体の高さ(足元〜頭)に重なっているか
		auto overlapBodyY = [&](float y)
		{
			return y - blockHalf < eyePos.y && y + blockHalf > eyePos.y - playerHeight;
		};

		// 今までは体に重なっていなかったのに、今回の動きで重なるなら、高さは動かさない
		if (overlapBodyY(targetPos.y) && !overlapBodyY(oldPos.y))
		{
			targetPos.y = oldPos.y;
		}
	}



	if (distSq(targetPos) < minDist * minDist && distSq(targetPos) < distSq(oldPos))
	{
		targetPos = oldPos;
	}

	return targetPos;
}

// 毎フレーム、「今、掴めるギミックブロックを狙っているか」を更新する
void BlockGrabber::UpdateGrabTarget(const Math::Vector3& playerPos, const Math::Matrix& playerRotMat)
{
	// 何かを持っている間は、探す必要がない
	if (m_spCarriedBlock)
	{
		m_hasGrabTarget = false;
		m_isTargetFar = false;
		SetGrabHighlightTarget(m_spCarriedBlock);
		return;
	}

	const GrabCandidate candidate = FindGrabCandidate(playerPos, playerRotMat);
	const bool reachable = candidate.obj && candidate.dist <= kGrabRange;

	m_hasGrabTarget = reachable;					// 緑にするのは、届く時だけ
	m_isTargetFar = candidate.obj && !reachable;	// 見えているが遠い
	SetGrabHighlightTarget(reachable ? candidate.obj : nullptr);
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

void BlockGrabber::StartGrabEffect()
{
	StopGrabEffect();	// 念のため、前のが残っていたら止める
	if (!m_spCarriedBlock) return;

	// ファイル名だけ渡せばよい(Asset/Data/Effect/ はフレームワークが自動で前に付ける)
	// isLoopは使わない：ループだと終わった時にハンドルが変わり、SetPosで追従できなくなるため
	m_wpGrabEffect = KdEffekseerManager::GetInstance().Play(
		"Grab.efkefc", m_spCarriedBlock->GetPos(), kGrabEffectSize);
}

void BlockGrabber::UpdateGrabEffect()
{
	if (!m_spCarriedBlock) return;

	const std::shared_ptr<KdEffekseerObject> spEffect = m_wpGrabEffect.lock();
	if (spEffect && spEffect->IsPlaying())
	{
		spEffect->SetPos(m_spCarriedBlock->GetPos());	// ブロックの動きについていく
	}
	else
	{
		// 再生が終わっていたら、もう一度再生する
		StartGrabEffect();
	}
}

void BlockGrabber::StopGrabEffect()
{
	if (const auto spEffect = m_wpGrabEffect.lock()) spEffect->StopEffect();
	m_wpGrabEffect.reset();
}