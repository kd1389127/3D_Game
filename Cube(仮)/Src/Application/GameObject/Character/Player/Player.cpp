#include "Player.h"

#include "../../../Scene/SceneManager.h"
#include "Component/BlockGrabber.h"

Player::Player() = default;

Player::~Player() = default;

void Player::Init()
{
	// 地面から補正分上げる
	m_pos.y += m_adjustHeight;

	m_pDebugWire = std::make_unique<KdDebugWireFrame>();

	m_upBlockGrabber = std::make_unique<BlockGrabber>();
}

void Player::Update()
{
	// 進行方向をクリア
	m_moveDir = Math::Vector3::Zero;

	// キャラ制御 移動処理
	if (GetAsyncKeyState('W') & 0x8000) { m_moveDir.z = 1.0f; }
	if (GetAsyncKeyState('S') & 0x8000) { m_moveDir.z = -1.0f; }
	if (GetAsyncKeyState('A') & 0x8000) { m_moveDir.x = -1.0f; }
	if (GetAsyncKeyState('D') & 0x8000) { m_moveDir.x = 1.0f; }

	// キャラ制御 ジャンプ処理
	if (GetAsyncKeyState(VK_SPACE) & 0x8000)
	{
		if (m_jumpKeyFlg == false)
		{
			m_jumpKeyFlg = true;
			m_gravity -= m_jumpPow;
		}
	}
	else
	{
		m_jumpKeyFlg = false;
	}

	// ★ ブロック操作処理をコンポーネントへ委譲
	if (m_upBlockGrabber)
	{
		m_upBlockGrabber->Update(m_pos, GetRotationMatrix());
	}

	// キャラ制御 (回転角度の情報を更新)
	if (!(GetAsyncKeyState(VK_LSHIFT) & 0x8000))
	{
		UpdateRotateByMouse();
	}

	// ベクトルの向きをY軸の回転行列で変換
	m_moveDir = m_moveDir.TransformNormal(m_moveDir, GetRotationYMatrix());

	// 確定した向き情報を正規化
	m_moveDir.Normalize();

	// 座標更新(横方向)
	m_pos += m_moveDir * m_moveSpeed;

	// 今フレームの縦方向の移動量を先に計算(まだm_posには反映しない)
	float verticalMove = -m_gravity; // 上昇中は正の値になる
	m_gravity += m_gravityPow;

	// 上昇中(ジャンプ中)は、頭上への当たり判定でクランプする
	if (verticalMove > 0.0f)
	{
		KdCollider::RayInfo ceilRay;
		ceilRay.m_pos = m_pos; // 頭くらいの高さに合わせて調整してください(必要ならY方向にオフセット追加)
		ceilRay.m_dir = Math::Vector3::Up;
		ceilRay.m_range = verticalMove;
		ceilRay.m_type = KdCollider::TypeGround | KdCollider::TypeBump;

		std::list<KdCollider::CollisionResult> ceilResults;
		for (auto& obj : SceneManager::Instance().GetObjList())
		{
			if (obj.get() == this) continue;
			obj->Intersects(ceilRay, &ceilResults);
		}

		if (!ceilResults.empty())
		{
			float minDist = verticalMove;
			for (auto& ret : ceilResults)
			{
				float dist = (ret.m_hitPos - ceilRay.m_pos).Length();
				if (dist < minDist) minDist = dist;
			}

			verticalMove = std::max(0.0f, minDist - 0.05f); // 天井の少し手前で止める
			m_gravity = 0.0f; // 頭をぶつけたら上昇を打ち切って落下に転じる
		}
	}

	// ここでまとめて縦移動を適用
	m_pos.y += verticalMove;


	// ワールド行列確定
	Math::Matrix transMat;
	transMat = Math::Matrix::CreateTranslation(m_pos);

	Math::Matrix rotMat;
	rotMat = GetRotationMatrix();

	m_mWorld = rotMat * transMat;
}

void Player::PostUpdate()
{
	ResolveWallCollsion();

	// レイ判定用のパラメーター
	KdCollider::RayInfo ray;

	// 段差の許容範囲 (0.5までの高さなら登れる)
	static const float enableStepHight = 0.5f;

	// レイの各パラメーター
	ray.m_pos = m_pos;
	ray.m_pos.y = ray.m_pos.y - m_adjustHeight + enableStepHight;
	ray.m_dir = Math::Vector3::Down;
	ray.m_range = m_gravity + enableStepHight;
	ray.m_type = KdCollider::TypeGround;

	// 衝突情報リスト
	std::list <KdCollider::CollisionResult> resultList;

	// 作成したレイ情報でオブジェクトリストと当たり判定
	for (auto& obj : SceneManager::Instance().GetObjList())
	{
		obj->Intersects(ray, &resultList);
	}

	// 衝突情報リストから一番近いオブジェクトを検出
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

	// 当たっていたら
	if (isHit)
	{
		m_pos = groundPos;
		m_pos.y += m_adjustHeight;
		m_gravity = 0;
	}

}

bool Player::IsCarryingBlock() const
{
	if (m_upBlockGrabber)
	{
		return m_upBlockGrabber->IsCarrying();
	}
	return false;
}

void Player::ResolveWallCollsion()
{
	// プレイヤーの体を表すBOXサイズ
	static const float bodyRadius = 3.0f;   // 横幅(半分)
	static const float bodyHeight = 10.0f;   // 身長

	static const float wallNormalYThreshold = 0.5f; // これ以上Y成分があれば床/天井とみなす
	static const float maxPushPerHit = bodyRadius;   // 異常値によるすっ飛び防止の上限

	const int resolveIteration = 4;

	for (int iteration = 0; iteration < resolveIteration; ++iteration)
	{
		Math::Vector3 totalPush = Math::Vector3::Zero;
		bool anyHit = false;

		// プレイヤー中心のBOXを作成(縦方向は足元〜頭の中間に合わせてオフセット)
		Math::Matrix boxMat = Math::Matrix::CreateTranslation(m_pos);
		Math::Vector3 boxOffset(0.0f, -m_adjustHeight + bodyHeight * 0.5f, 0.0f);
		Math::Vector3 boxHalfExtents(bodyRadius, bodyHeight * 0.5f, bodyRadius);

		// isOriented = false → 回転無視のAABBとして扱う(体の当たり判定はシンプルな方が安定)
		KdCollider::BoxInfo boxInfo(KdCollider::TypeBump, boxMat, boxOffset, boxHalfExtents, false);

		std::list<KdCollider::CollisionResult> resultList;
		for (auto& obj : SceneManager::Instance().GetObjList())
		{
			if (obj.get() == this) continue;
			obj->Intersects(boxInfo, &resultList);
		}

		// 同じ方向への重複pushを避けるため、方向ごとに「最大の押し出し量」だけ採用する
		std::unordered_map<int, Math::Vector3> bestPushByDir; // key: 押し出し方向の大まかな分類

		for (auto& ret : resultList)
		{
			Math::Vector3 normal = ret.m_hitNDir;
			if (normal.LengthSquared() < 0.0001f) continue;
			normal.Normalize();

			if (fabsf(normal.y) > wallNormalYThreshold) continue; // 床/天井は無視、壁のみ処理

			float overlap = std::min(ret.m_overlapDistance, maxPushPerHit);

			Math::Vector3 push = normal * overlap;
			push.y = 0.0f;

			// 法線を8方向くらいに丸めてキー化し、同方向の重複は大きい方だけ残す
			int dirKey = (int)(atan2f(normal.z, normal.x) * (4.0f / DirectX::XM_PI));
			auto it = bestPushByDir.find(dirKey);
			if (it == bestPushByDir.end() || it->second.LengthSquared() < push.LengthSquared())
			{
				bestPushByDir[dirKey] = push;
			}

			anyHit = true;
		}

		for (auto& kv : bestPushByDir)
		{
			totalPush += kv.second;
		}

		if (!anyHit) break;

		m_pos += totalPush;
	}
}

void Player::UpdateRotateByMouse()
{
	// マウスでカメラを回転させる処理
	POINT _nowPos;
	GetCursorPos(&_nowPos);

	POINT _mouseMove{};
	_mouseMove.x = _nowPos.x - m_fixMousePos.x;
	_mouseMove.y = _nowPos.y - m_fixMousePos.y;

	SetCursorPos(m_fixMousePos.x, m_fixMousePos.y);

	// 実際にカメラを回転させる処理(0.15はただの補正値)
	m_degAng.x += _mouseMove.y * 0.15f;
	m_degAng.y += _mouseMove.x * 0.15f;

	// 回転制御
	m_degAng.x = std::clamp(m_degAng.x, -75.f, 75.f);
}