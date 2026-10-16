#include "Player.h"

#include "../../../Scene/SceneManager.h"
#include "../../../Scene/StageData.h"
#include "Component/BlockGrabber.h"
#include "Component/BlockDestroyer.h"
#include "../../Block/BlockGridManager.h"
#include "../../UI/Mouse/Mouse.h"
#include "../../Collision/Collision.h"
#include "../../Map/Gimmick/Lift/Lift.h"

Player::Player() = default;
Player::~Player() = default;

// 初期化：ステージごとの開始位置(startPos)を受け取ってセットする
void Player::Init(const Math::Vector3& startPos, float groundHeight, float startYawDeg)
{
	m_pos = startPos;
	m_pos.y += m_adjustHeight;

	m_startPos = m_pos;

	// 開始時の向きをセット(左右=Y軸回転のみ指定、上下の見上げは0度から始める)
	m_degAng = Math::Vector3(0.0f, startYawDeg, 0.0f);
	m_startDegAng = m_degAng;

	m_upBlockGrabber = std::make_unique<BlockGrabber>();
	m_upBlockGrabber->Init(groundHeight); // ← groundHeightを渡す

	m_upBlockDestroyer = std::make_unique<BlockDestroyer>();
}

// 毎フレームの更新：入力受付・移動・重力・ゴール判定などをまとめて行う
void Player::Update()
{
	// 今フレームの進行方向を毎回リセットしてから、押されているキーに応じて組み立てる
	m_moveDir = Math::Vector3::Zero;

	// WASDキーで移動方向を決定(まだプレイヤーの向きは考慮していない、ローカルな方向)
	if (GetAsyncKeyState('W') & 0x8000) { m_moveDir.z = 1.0f; }
	if (GetAsyncKeyState('S') & 0x8000) { m_moveDir.z = -1.0f; }
	if (GetAsyncKeyState('A') & 0x8000) { m_moveDir.x = -1.0f; }
	if (GetAsyncKeyState('D') & 0x8000) { m_moveDir.x = 1.0f; }

	// スペースキーでジャンプ(押しっぱなしで連続ジャンプしないようフラグで1回だけ反応させる)
	if (GetAsyncKeyState(VK_SPACE) & 0x8000)
	{
		if (m_jumpKeyFlg == false)
		{
			m_jumpKeyFlg = true;
			m_gravity -= m_jumpPow; // 重力の値をマイナスにする＝上に跳ねる勢いを与える
		}
	}
	else
	{
		m_jumpKeyFlg = false;
	}
	
	// Shiftキーを押している間は視点回転を止め、マウスの固定も解いて自由に動かせるようにする
	// (スクショ撮影などで使う)
	bool isFreeMouse = (GetAsyncKeyState(VK_LSHIFT) & 0x8000) != 0;
	Mouse::Instance().SetFreeMove(isFreeMouse);

	if (!isFreeMouse)
	{
		UpdateRotateByMouse();
	}

	// デバッグ用：Pキーを押した瞬間の座標をログウィンドウに出す(ゴール位置決め等に使う)
	if (GetAsyncKeyState('P') & 0x8000)
	{
		KdDebugGUI::Instance().AddLog("PlayerPos: X=%.2f Y=%.2f Z=%.2f\n", m_pos.x, m_pos.y, m_pos.z);
	}

	// WASDで決めた「ローカルな向き」を、プレイヤーの向いている方向(Y軸回転)に合わせて変換する
	// これにより「Wキー＝常に見ている方向へ前進」という動きになる
	m_moveDir = m_moveDir.TransformNormal(m_moveDir, GetRotationYMatrix());

	// 長さを1に正規化(斜め移動が速くなりすぎないようにする)
	m_moveDir.Normalize();

	// 横方向の移動を確定(壁への当たり判定はまだしていない。PostUpdateで後から押し戻す方式)
	m_pos += m_moveDir * m_moveSpeed;

	if (m_upBlockGrabber)
	{
		m_upBlockGrabber->Update(m_pos, GetRotationMatrix());
	}
	
	// ブロック削除関連
	bool isCarrying = m_upBlockGrabber->IsCarrying();

	if (m_upBlockDestroyer)
	{
		if (m_isAimBlocking || isCarrying)
		{
			// エイム中/プレビュー中/持ち運び中は削除判定を止め、ハイライトも消す
			m_upBlockDestroyer->ClearTarget();
		}
		else
		{
			m_upBlockDestroyer->Update(m_pos, GetRotationMatrix());
		}
	}

	// 持っているブロックにプレイヤー自身がめり込んでいないかチェックして押し出す
	ResolvePushOutFromCarriedBlock();

	// ----- 縦方向(重力・ジャンプ)の移動量計算 -----
	// m_gravityは「下向きの勢い」を表す変数。マイナスならジャンプ中(上昇中)。
	float verticalMove = -m_gravity; // 上昇中は正の値になる
	m_gravity += m_gravityPow; // 毎フレーム重力を蓄積(だんだん落下が速くなる)

	// 上昇中(ジャンプ中)は、頭の上に天井がないかレイでチェックする
	if (verticalMove > 0.0f)
	{
		KdCollider::RayInfo ceilRay;
		ceilRay.m_pos = m_pos;
		ceilRay.m_dir = Math::Vector3::Up;
		ceilRay.m_range = verticalMove; // 今フレームで上昇する分だけ判定すればよい
		ceilRay.m_type = KdCollider::TypeGround | KdCollider::TypeBump;

		std::list<KdCollider::CollisionResult> ceilResults;

		std::vector<KdGameObject*> nearObjs;
		CollisionFilter::CollectAround(m_pos, 20.0f, nearObjs, this);

		for (KdGameObject* obj : nearObjs)
		{
			obj->Intersects(ceilRay, &ceilResults);
		}

		if (!ceilResults.empty())
		{
			// 天井に一番近い距離を探す
			float minDist = verticalMove;
			for (auto& ret : ceilResults)
			{
				float dist = (ret.m_hitPos - ceilRay.m_pos).Length();
				if (dist < minDist) minDist = dist;
			}

			// 天井の少し手前で上昇を止める(めり込み防止に0.05だけ余裕を持たせる)
			verticalMove = std::max(0.0f, minDist - 0.05f);
			m_gravity = 0.0f; // 頭をぶつけたら上昇をやめて落下に切り替える
		}
	}

	// 計算しておいた縦移動量をここでまとめて座標に反映する
	m_pos.y += verticalMove;

	// ワールド行列(座標＋回転)を確定させる
	Math::Matrix transMat = Math::Matrix::CreateTranslation(m_pos);
	Math::Matrix rotMat = GetRotationMatrix();
	m_mWorld = rotMat * transMat;

	// ゴールに触れているかを毎フレームチェック
	CheckGoal();
}

// 毎フレームの後処理：壁の押し出しと、地面へのスナップ(足元合わせ)を行う
void Player::PostUpdate()
{
	// 奈落の暗転で真っ暗になった瞬間なら、スタート位置に戻る
	if (SceneManager::Instance().ConsumeBlackoutEvent())
	{
		Respawn();
	}

	// 壁にめり込んでいたら押し出す(貫通防止)
	ResolveWallCollsion();

	// ----- 地面判定(足元にレイを飛ばして高さを合わせる) -----
	KdCollider::RayInfo ray;

	// 段差の許容範囲：これくらいの高さの段差なら、詰まらず登れるようにする
	static const float enableStepHight = 0.5f;

	// 足元より少し上から、下向きにレイを飛ばす(段差を登れるように少し高い位置から判定)
	ray.m_pos = m_pos;
	ray.m_pos.y = ray.m_pos.y - m_adjustHeight + enableStepHight;
	ray.m_dir = Math::Vector3::Down;
	ray.m_range = m_gravity + enableStepHight; // 落下速度が速いほど、判定範囲も伸ばす
	ray.m_type = KdCollider::TypeGround;

	const KdGameObject* pCarried = m_upBlockGrabber ? m_upBlockGrabber->GetCarriedBlock() : nullptr;

	std::list <KdCollider::CollisionResult> resultList;

	std::vector<KdGameObject*> nearObjs;
	CollisionFilter::CollectAround(m_pos, 20.0f, nearObjs, this);

	// リフト専用のレイ：足元より少し高い位置(kLiftStepHeight)から飛ばす
	// 角から斜めに入って足元が上面より少し下にあっても、上面に押し上げて乗せるため
	static const float kLiftStepHeight = 4.0f;		// ResolveWallCollsionのfootCutと同じ
	static const float kLiftTopTolerance = 1.0f;	// 「上面」とみなす高さの許容

	KdCollider::RayInfo liftRay = ray;
	liftRay.m_pos.y = m_pos.y - m_adjustHeight + kLiftStepHeight;
	liftRay.m_range = m_gravity + kLiftStepHeight;

	bool isHit = false;
	float maxOverLap = 0.0f;
	Math::Vector3 groundPos = Math::Vector3::Zero;
	const KdGameObject* pGroundObj = nullptr;

	for (KdGameObject* obj : nearObjs)
	{
		if (obj == pCarried) continue;

		// Liftなら、専用のレイを使う
		const Lift* pLiftObj = dynamic_cast<const Lift*>(obj);

		std::list<KdCollider::CollisionResult> objResults;
		obj->Intersects(pLiftObj ? liftRay : ray, &objResults);

		for (auto& ret : objResults)
		{
			// リフトは、上面(今のリフトの高さ)に当たった結果だけ採用する
			// 底の面や横の面に当たった結果は無視するので、中に埋まった状態で立たなくなる
			if (pLiftObj)
			{
				const Math::Vector3 hitPos = ret.m_hitPos;
				if (fabsf(hitPos.y - pLiftObj->GetTopY()) > kLiftTopTolerance) continue;
			}

			if (maxOverLap < ret.m_overlapDistance)
			{
				maxOverLap = ret.m_overlapDistance;
				groundPos = ret.m_hitPos;
				pGroundObj = obj;
				isHit = true;
			}
		}
	}

	// 地面に当たっていたら、そこに足を合わせて重力(落下速度)をリセットする
	if (isHit)
	{
		m_pos = groundPos;
		m_pos.y += m_adjustHeight; // 頭基準の座標に戻す
		m_gravity = 0;

		// リフトに乗っていたら、リフトが今フレームに動いた横方向の量だけ一緒に動く
		// (縦は毎フレームこの足元レイで高さが合うので、足さなくてよい)
		if (const Lift* pLift = dynamic_cast<const Lift*>(pGroundObj))
		{
			m_pos.x += pLift->GetDelta().x;
			m_pos.z += pLift->GetDelta().z;
		}
	}

	// ----- 奈落判定 -----
	// スタート地点のY座標を基準に、そこから大きく下に落ちたらリスポーン
	constexpr float fallLimitOffset = -50.0f;
	if (m_pos.y < m_startPos.y + fallLimitOffset)
	{
		SceneManager::Instance().RequestFadeOnly();
		return;
	}
}

// 今ブロックを持っているかどうかを他クラス(Magicwandなど)に伝えるための関数
bool Player::IsCarryingBlock() const
{
	if (m_upBlockGrabber)
	{
		return m_upBlockGrabber->IsCarrying();
	}
	return false;
}

bool Player::IsGrabActive() const
{
	return m_upBlockGrabber && m_upBlockGrabber->IsGrabActive();
}

bool Player::IsGrabTargetFar() const
{
	return m_upBlockGrabber && m_upBlockGrabber->IsGrabTargetFar();
}

int Player::GetDropBlockedId() const
{
	return m_upBlockGrabber ? m_upBlockGrabber->GetDropBlockedId() : 0;
}

bool Player::HasDeleteTarget() const
{
	return m_upBlockDestroyer && m_upBlockDestroyer->HasTarget();
}

bool Player::TryDeleteTarget()
{
	if (!m_upBlockDestroyer) return false;
	return m_upBlockDestroyer->TryDeleteTarget();
}

void Player::Respawn()
{
	m_pos = m_startPos;
	m_degAng = m_startDegAng;	// 向きも戻す(落ちた方向を向いたままだと分かりにくい)
	m_gravity = 0.0f;			// 落下で溜まった勢いを捨てる。これを忘れると着地で突き抜ける
}

// ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== =====
// 壁にめり込んでいる分だけプレイヤーを押し出す処理(貫通防止の本体)
// プレイヤーを「縦長の箱(BOX)」とみなして、マップの壁メッシュとの重なりを調べ、
// 重なっている方向と量に応じて押し戻す
// ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== =====
void Player::ResolveWallCollsion()
{
	// プレイヤーの体を表すBOXサイズ
	static const float bodyRadius = 3.0f;   // 横幅(半分)
	static const float bodyHeight = 10.0f;  // 身長
	static const float footCut    = 4.0f;	// 足元から、この高さ未満は壁として扱わない(bodyRadius以上にする)

	static const float wallNormalYThreshold = 0.5f; // 法線のY成分がこれを超えたら「床/天井」とみなして無視する
	static const float maxPushPerHit = bodyRadius;   // 1回の押し出し量に上限を設け、異常なすっ飛びを防ぐ

	// 1回の判定だけだと、複数の壁(角など)に同時に押されるケースで押し出しきれないことがあるため、
	// 複数回(4回)繰り返して少しずつ正しい位置に収束させる
	const int resolveIteration = 4;

	// 体の半径3 + 押し出しの余裕を見て20(ブロックの大きさ7は関数側で足される)
	std::vector<KdGameObject*> nearObjs;
	CollisionFilter::CollectAround(m_pos, 20.0f, nearObjs, this);

	for (int iteration = 0; iteration < resolveIteration; ++iteration)
	{
		Math::Vector3 totalPush = Math::Vector3::Zero;
		bool anyHit = false;

		// プレイヤーの体を表すBOX(当たり判定用の箱)を作成
		Math::Matrix boxMat = Math::Matrix::CreateTranslation(m_pos);
		const float boxHeight = bodyHeight - footCut;
		Math::Vector3 boxOffset(0.0f, -m_adjustHeight + footCut + boxHeight * 0.5f, 0.0f);
		Math::Vector3 boxHalfExtents(bodyRadius, boxHeight * 0.5f, bodyRadius);
		
		// isOriented = false → 回転を考慮しないシンプルな箱(AABB)として判定する
		KdCollider::BoxInfo boxInfo(KdCollider::TypeBump, boxMat, boxOffset, boxHalfExtents, false);

		std::list<KdCollider::CollisionResult> resultList;
		for (KdGameObject* obj : nearObjs)
		{
			obj->Intersects(boxInfo, &resultList);
		}

		// 同じ方向から複数回押されるのを防ぐため、方向ごとに「一番強い押し出し」だけを採用する
		std::unordered_map<int, Math::Vector3> bestPushByDir;

		for (auto& ret : resultList)
		{
			Math::Vector3 normal = ret.m_hitNDir;
			if (normal.LengthSquared() < 0.0001f) continue;
			normal.Normalize();

			// 法線が上下方向に近い(＝床や天井)なら、壁扱いせずスキップする
			if (fabsf(normal.y) > wallNormalYThreshold) continue;

			// 押し出し量が異常に大きくならないよう上限をかける
			float overlap = std::min(ret.m_overlapDistance, maxPushPerHit);

			Math::Vector3 push = normal * overlap;
			push.y = 0.0f; // 横方向にしか押し出さない(縦方向はPostUpdateの地面判定に任せる)

			// 法線の向きをおおよそ8方向くらいに分類し、同じ方向からの重複押し出しをまとめる
			int dirKey = (int)(atan2f(normal.z, normal.x) * (4.0f / DirectX::XM_PI));
			auto it = bestPushByDir.find(dirKey);
			if (it == bestPushByDir.end() || it->second.LengthSquared() < push.LengthSquared())
			{
				bestPushByDir[dirKey] = push;
			}

			anyHit = true;
		}

		// 各方向の押し出しを合計して、実際の移動量とする
		for (auto& kv : bestPushByDir)
		{
			totalPush += kv.second;
		}

		// 何にも当たっていなければ、それ以上繰り返す必要はないので打ち切る
		if (!anyHit) break;

		m_pos += totalPush;
	}
}

// ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== =====
// ゴールに触れたかどうかを判定し、触れていれば次のステージ(またはリザルト)へ切り替える
// ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== =====
void Player::CheckGoal()
{
	static const float checkRadius = 5.0f;

	// プレイヤーの体の中心あたりに判定用の球を置く
	Math::Vector3 center = m_pos;
	center.y += (-m_adjustHeight + 5.0f);

	// TypeEvent(ゴールなどのイベント用当たり判定)とだけ判定する球を作成
	KdCollider::SphereInfo sphereInfo(KdCollider::TypeEvent, center, checkRadius);

	// 球で判定するので、中心と半径で絞る
	std::vector<KdGameObject*> nearObjs;
	CollisionFilter::CollectAround(center, checkRadius, nearObjs, this);

	std::list<KdCollider::CollisionResult> resultList;
	for (KdGameObject* obj : nearObjs)
	{
		obj->Intersects(sphereInfo, &resultList);
	}

	// 何かTypeEventの物(＝ゴール)に触れていたら
	if (!resultList.empty())
	{
		int nextStage = SceneManager::Instance().GetCurrentStage() + 1;

		if (nextStage < g_stageCount)
		{
			SceneManager::Instance().RequestChangeScene(SceneManager::SceneType::Game, nextStage);
		}
		else
		{
			// 全ステージクリア→リザルトへ(ステージ番号は0に戻す)
			SceneManager::Instance().RequestChangeScene(SceneManager::SceneType::Result, 0);
		}
	}
}

// ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== =====
// 持っているブロックの中に、プレイヤー自身が入り込んでしまっていたら押し出す
// (視点を下げてブロックを覗き込んだ時などに、体が重なってしまうのを防ぐ)
// ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== =====
void Player::ResolvePushOutFromCarriedBlock()
{
	if (!m_upBlockGrabber || !m_upBlockGrabber->IsCarrying()) return;

	const Math::Vector3 blockPos = m_upBlockGrabber->GetCarriedBlockPos();

	constexpr float blockHalfSize = BlockGridManager::GridSize * 0.5f; // ブロックの半サイズ(4.0f)
	constexpr float playerRadius  = 3.0f;	// ResolveWallCollsionのbodyRadiusと同じ値
	constexpr float jumpReach     = 15.0f;	// ジャンプで届く高さ(約12.5)+余裕

	// m_posは頭の高さ(足元は m_adjustHeight だけ下)
	const float headY = m_pos.y;
	const float feetY = m_pos.y - m_adjustHeight;

	// 高さの条件:
	//  ・頭がブロックの底より上にある(頭の上に浮いているブロックの下は通れる)
	//  ・足がブロックの上面+ジャンプの届く高さより下にある(=ブロックの上にも柱が伸びている扱い)
	const bool inColumnHeight = headY > blockPos.y - blockHalfSize
								&& feetY < blockPos.y + blockHalfSize + jumpReach;
	if (!inColumnHeight) return;

	const Math::Vector3 diff = m_pos - blockPos;
	const float overlapX = (blockHalfSize + playerRadius) - fabsf(diff.x);
	const float overlapZ = (blockHalfSize + playerRadius) - fabsf(diff.z);

	// 横(X・Z)で重なっていたら、浅い方向へ押し出す。Yは押し出さない
	if (overlapX > 0.0f && overlapZ > 0.0f)
	{
		if (overlapX <= overlapZ)
		{
			m_pos.x += (diff.x >= 0.0f ? overlapX : -overlapX);
		}
		else
		{
			m_pos.z += (diff.z >= 0.0f ? overlapZ : -overlapZ);
		}
	}
}

// マウスの移動量からカメラ(プレイヤーの向き)を回転させる処理
// カーソルの固定(中央に戻す処理)はMouseクラスが担当するので、ここでは移動量を受け取るだけ
void Player::UpdateRotateByMouse()
{
	// Mouseのdeltaは「右が+、上が+」
	Math::Vector2 delta = Mouse::Instance().GetDelta();

	// 左右(ヨー)：右に動かしたら右を向く
	m_degAng.y += delta.x * 0.15f;

	// 上下(ピッチ)：元のコードは「下が+」だったので、上が+のdeltaは符号を反転する
	m_degAng.x -= delta.y * 0.15f;

	// 上下の見上げ/見下ろし角度が真上/真下を超えないよう制限する
	m_degAng.x = std::clamp(m_degAng.x, -75.f, 75.f);
}