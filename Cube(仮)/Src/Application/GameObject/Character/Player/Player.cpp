#include "Player.h"

#include "../../../Scene/SceneManager.h"
void Player::Init()
{
	// 地面から補正分上げる
	m_pos.y += m_adjustHeight;

	m_pDebugWire = std::make_unique<KdDebugWireFrame>();
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

	ResolveWallCollsion();

}

void Player::ResolveWallCollsion()
{
	KdCollider::SphereInfo sphereInfo(KdCollider::TypeBump, m_pos, 3.0f); // 3.0fはプレイヤーの太さに合わせて調整

	std::list<KdCollider::CollisionResult> resultList;

	for (auto& obj : SceneManager::Instance().GetObjList())
	{
		if (obj.get() == this) continue; // 自分自身は除外
		obj->Intersects(sphereInfo, &resultList);
	}

	for (auto& ret : resultList)
	{
		Math::Vector3 normal = ret.m_hitNDir;
		if (normal.LengthSquared() < 0.0001f) continue;
		normal.Normalize();

		// 面が上向き(床・ブロック上面) → その面に沿って上に押し出す(乗る)
		if (normal.y > 3.0f)
		{
			m_pos += normal * ret.m_overlapDistance;
			m_gravity = 0.0f;
		}
		// 面が横向き(壁・ブロック側面) → 横方向だけ押し出す(縦はそのまま)
		else
		{
			Math::Vector3 pushDir = normal;
			pushDir.y = 0;
			if (pushDir.LengthSquared() > 0.0001f)
			{
				pushDir.Normalize();
				m_pos += pushDir * ret.m_overlapDistance;
			}
		}
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
	m_degAng.x = std::clamp(m_degAng.x, -45.f, 45.f);
}
