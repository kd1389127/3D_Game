#include "Player.h"

#include "../../../Scene/SceneManager.h"
#include "../../../Scene/StageData.h"
#include "Component/BlockGrabber.h"
#include "../../Block/BlockGridManager.h"

Player::Player() = default;
Player::~Player() = default;

// 初期化：ステージごとの開始位置(startPos)を受け取ってセットする
void Player::Init(const Math::Vector3& startPos, float groundHeight)
{
	m_pos = startPos;
	m_pos.y += m_adjustHeight;

	m_upBlockGrabber = std::make_unique<BlockGrabber>();
	m_upBlockGrabber->Init(groundHeight); // ← groundHeightを渡す
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

	// Shiftキーを押している間はマウスでの視点回転を止める(ブロック操作中などに使う想定)
	if (!(GetAsyncKeyState(VK_LSHIFT) & 0x8000))
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

	// ブロックを掴む/置く/持ち運ぶ処理を、専用のコンポーネントに任せる
	if (m_upBlockGrabber)
	{
		m_upBlockGrabber->Update(m_pos, GetRotationMatrix());
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
		for (auto& obj : SceneManager::Instance().GetObjList())
		{
			if (obj.get() == this) continue; // 自分自身とは判定しない
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

	std::list <KdCollider::CollisionResult> resultList;

	for (auto& obj : SceneManager::Instance().GetObjList())
	{
		obj->Intersects(ray, &resultList);
	}

	// 複数の地面候補の中から「一番近い(重なりが大きい)」ものを採用する
	bool isHit = false;
	float maxOverLap = 0.0f;
	Math::Vector3 groundPos = Math::Vector3::Zero;

	for (auto& ret : resultList)
	{
		if (maxOverLap < ret.m_overlapDistance)
		{
			maxOverLap = ret.m_overlapDistance;
			groundPos = ret.m_hitPos;
			isHit = true;
		}
	}

	// 地面に当たっていたら、そこに足を合わせて重力(落下速度)をリセットする
	if (isHit)
	{
		m_pos = groundPos;
		m_pos.y += m_adjustHeight; // 頭基準の座標に戻す
		m_gravity = 0;
	}
}

// ブロック持ち上げ中のプレビュー描画をコンポーネントに委譲
void Player::DrawLit()
{
	if (m_upBlockGrabber)
	{
		m_upBlockGrabber->DrawPreview();
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

	static const float wallNormalYThreshold = 0.5f; // 法線のY成分がこれを超えたら「床/天井」とみなして無視する
	static const float maxPushPerHit = bodyRadius;   // 1回の押し出し量に上限を設け、異常なすっ飛びを防ぐ

	// 1回の判定だけだと、複数の壁(角など)に同時に押されるケースで押し出しきれないことがあるため、
	// 複数回(4回)繰り返して少しずつ正しい位置に収束させる
	const int resolveIteration = 4;

	for (int iteration = 0; iteration < resolveIteration; ++iteration)
	{
		Math::Vector3 totalPush = Math::Vector3::Zero;
		bool anyHit = false;

		// プレイヤーの体を表すBOX(当たり判定用の箱)を作成
		Math::Matrix boxMat = Math::Matrix::CreateTranslation(m_pos);
		Math::Vector3 boxOffset(0.0f, -m_adjustHeight + bodyHeight * 0.5f, 0.0f); // 足元〜頭の中間に合わせる
		Math::Vector3 boxHalfExtents(bodyRadius, bodyHeight * 0.5f, bodyRadius);

		// isOriented = false → 回転を考慮しないシンプルな箱(AABB)として判定する
		KdCollider::BoxInfo boxInfo(KdCollider::TypeBump, boxMat, boxOffset, boxHalfExtents, false);

		std::list<KdCollider::CollisionResult> resultList;
		for (auto& obj : SceneManager::Instance().GetObjList())
		{
			if (obj.get() == this) continue;
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

	std::list<KdCollider::CollisionResult> resultList;
	for (auto& obj : SceneManager::Instance().GetObjList())
	{
		if (obj.get() == this) continue;
		obj->Intersects(sphereInfo, &resultList);
	}

	// 何かTypeEventの物(＝ゴール)に触れていたら
	if (!resultList.empty())
	{
		int nextStage = SceneManager::Instance().GetCurrentStage() + 1;

		if (nextStage < g_stageCount)
		{
			SceneManager::Instance().SetCurrentStage(nextStage);
			SceneManager::Instance().SetNextScene(SceneManager::SceneType::Game);
			SceneManager::Instance().ReloadScene();		// 型がGameのままでも強制的に作り直す
		}
		else
		{
			SceneManager::Instance().SetCurrentStage(0); // 全ステージクリア→次は最初からなのでリセット
			SceneManager::Instance().SetNextScene(SceneManager::SceneType::Result);
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

	Math::Vector3 blockPos = m_upBlockGrabber->GetCarriedBlockPos();

	constexpr float blockHalfSize = BlockGridManager::GridSize * 0.5f; // ブロックの半サイズ(4.0f)
	const float playerRadius = 3.0f;      // ResolveWallCollsionのbodyRadiusと同じ値
	const float playerHalfHeight = 5.0f;  // ResolveWallCollsionのbodyHeight(10.0f)の半分

	// プレイヤーの当たり判定の中心座標(足元基準のm_posから、体の中心分だけ持ち上げる)
	Math::Vector3 playerCenter = m_pos;
	playerCenter.y += (-m_adjustHeight + playerHalfHeight);

	Math::Vector3 diff = playerCenter - blockPos;

	// XYZそれぞれの軸で「どれだけ重なっているか」を計算(マイナスなら重なっていない)
	float overlapX = (blockHalfSize + playerRadius) - fabsf(diff.x);
	float overlapY = (blockHalfSize + playerHalfHeight) - fabsf(diff.y);
	float overlapZ = (blockHalfSize + playerRadius) - fabsf(diff.z);

	// 3軸すべてで重なっていて、初めて実際に箱同士が重なっていると言える
	if (overlapX > 0.0f && overlapY > 0.0f && overlapZ > 0.0f)
	{
		// 重なりが浅い方の軸(X or Z)だけ押し出す
		// Yは押し出さない＝ブロックに押されて浮いたり沈んだりしないようにする
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
void Player::UpdateRotateByMouse()
{
	POINT _nowPos;
	GetCursorPos(&_nowPos); // 現在のマウスカーソル位置(画面座標)を取得

	// 前フレームで固定した中心位置(m_fixMousePos)からの移動量を計算
	POINT _mouseMove{};
	_mouseMove.x = _nowPos.x - m_fixMousePos.x;
	_mouseMove.y = _nowPos.y - m_fixMousePos.y;

	// カーソルを毎フレーム中心位置に戻す(FPSゲームでよくある「無限にマウスを動かせる」仕組み)
	SetCursorPos(m_fixMousePos.x, m_fixMousePos.y);

	// マウスの移動量を回転角度に変換して加算する(0.15は感度調整用の係数)
	m_degAng.x += _mouseMove.y * 0.15f; // 上下(ピッチ)
	m_degAng.y += _mouseMove.x * 0.15f; // 左右(ヨー)

	// 上下の見上げ/見下ろし角度が真上/真下を超えないよう制限する
	m_degAng.x = std::clamp(m_degAng.x, -75.f, 75.f);
}