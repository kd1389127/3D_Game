#include "Magicwand.h"

#include "../../../main.h"                     
#include "../../../Scene/SceneManager.h"
#include "../../Bullet/Bullet.h"
#include "../../Character/Player/Player.h"
#include "../../Block/BlockGridManager.h"
#include "../../Block/NormalBlock/NormalBlock.h" 
#include "../../Map/Ground/Ground.h"
#include "../../Magic/MagicManager.h"

namespace
{
	// ===================================================
	// レイ(origin, dir)が、center±halfExtentsのAABBに当たるか判定する(スラブ法)
	// 当たった場合、出会うまでの距離(tOut)を返す
	// ===================================================
	bool IntersectRayAABB(const Math::Vector3& origin, const Math::Vector3& dir,
		const Math::Vector3& center, const Math::Vector3& halfExtents, float maxDist,
		float* tOut = nullptr, Math::Vector3* normalOut = nullptr)
	{
		Math::Vector3 boxMin = center - halfExtents;
		Math::Vector3 boxMax = center + halfExtents;

		float tMin = 0.0f;
		float tMax = maxDist;
		int   hitAxis = -1;
		float hitSign = 1.0f;

		const float* originArr = &origin.x;
		const float* dirArr = &dir.x;
		const float* boxMinArr = &boxMin.x;
		const float* boxMaxArr = &boxMax.x;

		for (int axis = 0; axis < 3; ++axis)
		{
			float o = originArr[axis];
			float d = dirArr[axis];
			float mn = boxMinArr[axis];
			float mx = boxMaxArr[axis];

			if (fabsf(d) < 1e-6f)
			{
				if (o < mn || o > mx) return false;
				continue;
			}

			float t1 = (mn - o) / d;
			float t2 = (mx - o) / d;
			if (t1 > t2) std::swap(t1, t2);

			if (t1 > tMin)
			{
				tMin = t1;
				hitAxis = axis;
				hitSign = (d > 0.0f) ? -1.0f : 1.0f; // ★入った面の外向き法線の符号
			}
			tMax = std::min(tMax, t2);

			if (tMin > tMax) return false;
		}

		if (tOut) *tOut = tMin;
		if (normalOut)
		{
			*normalOut = Math::Vector3::Zero;
			if (hitAxis >= 0) (&normalOut->x)[hitAxis] = hitSign;
		}
		return true;
	}
}

// 初期化：モデルの読み込みと、親(プレイヤー)から見た相対位置を設定する
void Magicwand::Init()
{
	if (!m_spModel)
	{
		m_spModel = std::make_shared<KdModelWork>();
		m_spModel->SetModelData("Asset/Models/Weapon/Magicwand/MagicwandBlue.gltf");

		if (!m_pDebugWire)
		{
			m_pDebugWire = std::make_unique<KdDebugWireFrame>();
		}

		// 杖を大きく見せるためのスケール(1.5倍程度から調整)
		m_scaleMat = Math::Matrix::CreateScale(1.3f, 1.3f, 1.3f);

		// 杖を傾けて構えているように見せるための回転
		// X軸：前後の傾き(先端を少し奥に倒す)　Z軸：左右の傾き(内側に少し倒す)
		Math::Matrix rot =
			Math::Matrix::CreateRotationX(DirectX::XMConvertToRadians(20.0f)) *  // 前後の傾きを少し強める
			Math::Matrix::CreateRotationY(DirectX::XMConvertToRadians(15.0f)) *  // 横方向にひねる
			Math::Matrix::CreateRotationZ(DirectX::XMConvertToRadians(30.0f));   // 左右の傾きも少し強める

		// 親(プレイヤー)から見た「杖本体」の相対位置(手元に構えている位置)
		// 位置も右下寄りに調整
		Math::Matrix pos = Math::Matrix::CreateTranslation(0.6f, -0.7f, 0.6f);

		// 親(プレイヤー)から見た「杖本体」の相対位置(手元に構えている位置)
		m_localMat = rot * pos;

		// 杖本体から見た「銃口(発射位置)」の相対位置に、さらに親からの相対位置を掛け合わせる
		// (＝結果的に「親から見た銃口の相対位置」になる)
		m_localMuzzleMat = Math::Matrix::CreateTranslation(0.2f, 0.7f, 0.7f);
		m_localMuzzleMat = m_localMuzzleMat * m_localMat;
	}
}

void Magicwand::Update()
{
	// 親オブジェクト(プレイヤー)を取得(weak_ptrなのでlock()して安全に使う)
	const std::shared_ptr<const KdGameObject> spParent = m_wpParent.lock();

	// 親がブロックを持ち上げ中かどうかを確認する(持っている間は発射できないようにするため)
	bool isCarrying = false;
	bool isDeleteMode = false;
	if (spParent)
	{
		auto player = std::dynamic_pointer_cast<const Player>(spParent);
		if (player)
		{
			if (player->IsCarryingBlock())   isCarrying = true;
			if (player->IsBlockDeleteMode()) isDeleteMode = true;
		}
	}

	// 親(プレイヤー)のワールド行列を取得(なければ単位行列＝原点扱い)
	Math::Matrix parentMat = Math::Matrix::Identity;
	if (spParent)
	{
		parentMat = spParent->GetMatrix();
	}

	// 銃口(発射位置)のワールド座標を計算する
	Math::Vector3 muzzlePos = (m_localMuzzleMat * parentMat).Translation();

	// ----- 今フレームのボタン状態と、押した/離した瞬間の判定 -----
	bool rightDownNow = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
	bool leftDownNow  = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;

	bool rightPressed  = rightDownNow && !m_rightDownPrev;	// 右クリックを押した瞬間
	bool rightReleased = !rightDownNow && m_rightDownPrev;	// 右クリックを離した瞬間
	bool leftPressed   = leftDownNow && !m_leftDownPrev;	//左クリックを押した瞬間

	switch (m_state)
	{
		// ---------------------------------------------------
		// 通常状態：右クリックを押した瞬間、エイムモードへ入る
		// ---------------------------------------------------
		case WandState::Idle:
		{
			if (rightPressed && !isCarrying && !isDeleteMode)
			{
				if (MagicManager::Instance().CanCast())
				{
					EnterAimMode();
					// 押した瞬間から即座にプレビューが出るよう、同じフレームで1回更新しておく
					UpdateAimMode(muzzlePos, parentMat);
				}
				else
				{
					MagicManager::Instance().MarkAttemptFailed();
				}
			}
			else if (leftPressed && !isCarrying && !isDeleteMode)
			{
				if (MagicManager::Instance().CanCast())
				{
					// 左クリック単発：エイムなしで即座に1個だけ生成する
					SingleShot(muzzlePos, parentMat);
				}
				else
				{
					MagicManager::Instance().MarkAttemptFailed();
				}
			}
			break;
		}

		// ---------------------------------------------------
		// エイムモード：長押し中、レイを飛ばし続けてプレビューを追従させる
		// ---------------------------------------------------
		case WandState::Aiming:
		{
			UpdateAimMode(muzzlePos, parentMat);

			// ズーム判定用に経過フレームを数える
			m_aimHoldFrames++;

			// 右クリックを離したら、狙っている位置で固定して調整モードへ
			if (rightReleased)
			{
				if (m_hasValidAim)
				{
					ClearFaceHighlight();	// 確定した瞬間、面ハイライトは役目を終える
					m_state = WandState::Adjusting;
				}
				else
				{
					// 何も狙えていない状態で離した場合はキャンセル扱い
					CancelAim();
				}
			}
			break;
		}

		// ---------------------------------------------------
		// 調整モード：プレビュー位置は固定。ホイールで段数調整
		// 左クリックで確定発射、右クリックでキャンセル
		// ---------------------------------------------------
		case WandState::Adjusting:
		{
			UpdateHighlight(muzzlePos, parentMat);	// 毎フレーム、狙っているプレビューを判定

			// ----- 右クリックでキャンセル -----
			if (rightPressed)
			{
				CancelAim();
				break;
			}

			// ----- 左クリックで確定発射 -----
			if (leftPressed)
			{
				ConfirmStack(muzzlePos,parentMat);
				break;
			}


			// ----- ホイールで段数変更 -----
			int wheelValue = Application::Instance().GetMouseWheelValue();

			if (wheelValue != 0)
			{
				// 壁(水平方向)へのせり出しは、伸びる向きがプレイヤー側(手前)になるため、
				// 床/天井(垂直方向)とは逆にホイールの符号を反転させて感覚を合わせる
				int effectiveWheel = wheelValue;
				if (fabsf(m_aimDir.y) < 0.5f) // Y成分が小さい＝水平方向(壁)
				{
					effectiveWheel = -wheelValue;
				}

				int maxAllowed = std::min(m_maxStackCount, MagicManager::Instance().GetRemainingCasts());
				int newCount = std::clamp(m_stackCount + (effectiveWheel > 0 ? 1 : -1), 1, maxAllowed);

				if (newCount != m_stackCount)
				{
					m_stackCount = newCount;
					RebuildPreview();
				}
			}
			break;
		}

	}

	// 次フレームの判定用に、今フレームのボタン状態を保存しておく
	m_rightDownPrev = rightDownNow;
	m_leftDownPrev  = leftDownNow;

	UpdateSwingAnim();

	// 基底クラス(WeaponBase)の更新処理を呼んで、ワールド行列などを確定させる
	WeaponBase::Update();
}

void Magicwand::StartSwingAnim()
{
	m_isSwinging = true;
	m_SwingFram = 0;
}

void Magicwand::UpdateSwingAnim()
{
	if (!m_isSwinging)
	{
		m_animMat = Math::Matrix::Identity;
		return;
	}

	m_SwingFram++;
	float t = m_SwingFram / (float)m_swingDuration;

	if (t >= 1.0f)
	{
		m_isSwinging = false;
		m_animMat = Math::Matrix::Identity;
		return;
	}

	// 0→1→0と滑らかに変化するカーブ(振り上げて、振り下ろして戻る動き)
	float swingT = sinf(t * DirectX::XM_PI);

	// X軸回転で「前に振り下ろす」動きを表現(角度はお好みで調整)
	float angle = DirectX::XMConvertToRadians(45.0f) * swingT;

	m_animMat = Math::Matrix::CreateRotationX(angle);

}

// ===================================================
// Idle状態での左クリック：エイムなしで即座に1個だけブロックを生成する(簡易射撃)
// UpdateAimMode/ConfirmStackと同じレイキャストロジックだが、
// プレビューや段数調整を挟まず、その場で1個だけ生成する点が異なる
// ===================================================
void Magicwand::SingleShot(const Math::Vector3& muzzlePos, const Math::Matrix& parentMat)
{
	KdCollider::RayInfo rayInfo;
	rayInfo.m_pos = muzzlePos;
	rayInfo.m_dir = parentMat.Backward();
	rayInfo.m_range = 1000.0f;
	rayInfo.m_type = KdCollider::TypeGround | KdCollider::TypeBump;

	std::list<KdCollider::CollisionResult> resultList;
	for (auto& obj : SceneManager::Instance().GetObjList())
	{
		obj->Intersects(rayInfo, &resultList);
	}

	bool  isHit		 = false;
	float minDistSqr = FLT_MAX;
	Math::Vector3 hitPos    = Math::Vector3::Zero;
	Math::Vector3 hitNormal = Math::Vector3::Up;

	for (auto& ret : resultList)
	{
		float distSqr = (ret.m_hitPos - muzzlePos).LengthSquared();
		if (distSqr < minDistSqr)
		{
			minDistSqr = distSqr;
			hitPos	   = ret.m_hitPos;
			hitNormal  = ret.m_hitNDir;
			isHit      = true;
		}
	}

	// 何にも当たらなかった場合は、レイの最大距離まで飛んだ先を仮の着弾点にする
	Math::Vector3 targetPos = isHit ? hitPos : (muzzlePos + rayInfo.m_dir * rayInfo.m_range);

	auto bullet = std::make_shared<Bullet>();

	if (isHit)
	{
		// パーティクルが作れたらパーティクルを呼ぶように
		// 何にも当たらなかった：弾は飛ぶが何も生成しない
		bullet->Init(muzzlePos, targetPos, hitNormal,
			[](const Math::Vector3&, const Math::Vector3&) {/* 何もしない */});
	}
	else
	{
		// 何にも当たらなかった：弾は飛ぶが何も生成しない
		bullet->Init(muzzlePos, targetPos, hitNormal,
			[](const Math::Vector3&, const Math::Vector3&) {/* 何もしない */});
	}

	SceneManager::Instance().AddObject(bullet);

	StartSwingAnim();
}

// エイムモード開始：段数・狙い情報をリセットする
void Magicwand::EnterAimMode()
{
	m_state = WandState::Aiming;
	m_stackCount = 1;
	m_hasValidAim = false;
	m_aimBaseCell = Math::Vector3::Zero;
	m_aimDir = Math::Vector3::Up;
	m_aimHoldFrames = 0;
}

// ===================================================
// シーン上のオブジェクトからGroundを探し、その壁コライダーを返す
// (毎回検索するのはやや非効率、着弾時にしか呼ばれないので許容範囲)
// ===================================================
std::shared_ptr<Ground> Magicwand::FindGround() const
{
	for (auto& obj : SceneManager::Instance().GetObjList())
	{
		auto ground = std::dynamic_pointer_cast<Ground>(obj);
		if (ground)
		{
			return ground;
		}
	}
	return nullptr;
}

// ===================================================
// エイムモード中、毎フレーム呼ばれる入力処理
// ===================================================
void Magicwand::UpdateAimMode(const Math::Vector3& muzzlePos, const Math::Matrix& parentMat)
{
	// ----- 視線方向にレイを飛ばし、狙っている場所を毎フレーム更新 -----
	KdCollider::RayInfo rayInfo;
	rayInfo.m_pos = muzzlePos;
	rayInfo.m_dir = parentMat.Backward(); // プレイヤーの前方向
	rayInfo.m_range = 1000.0f;            // 十分に遠くまで判定する
	rayInfo.m_type = KdCollider::TypeGround | KdCollider::TypeBump;

	std::list<KdCollider::CollisionResult> resultList;

	// 当たった候補の中から一番近いものを選ぶ
	bool isHit = false;
	float minDistSqr = FLT_MAX;
	Math::Vector3 hitPos = Math::Vector3::Zero;
	Math::Vector3 hitNormal = Math::Vector3::Up;
	std::shared_ptr<KdGameObject> hitObj = nullptr;		//当たったオブジェクトを覚える

	for (auto& obj : SceneManager::Instance().GetObjList())
	{
		// オブジェクトごとに分けて判定
		std::list<KdCollider::CollisionResult> localResult;
		if (!obj->Intersects(rayInfo, &localResult)) continue;

		for (auto& ret : localResult)
		{
			float distSqr = (ret.m_hitPos - muzzlePos).LengthSquared(); // 平方根計算を省いて軽量化

			if (distSqr < minDistSqr)
			{
				minDistSqr = distSqr;
				hitPos = ret.m_hitPos;
				hitNormal = ret.m_hitNDir;
				hitObj = obj;
				isHit = true;
			}
		}
	}
	if (isHit)
	{
		KdDebugGUI::Instance().AddLog(
			"[Wand] hitPos: X=%.2f Y=%.2f Z=%.2f / hitNormal: X=%.2f Y=%.2f Z=%.2f\n",
			hitPos.x, hitPos.y, hitPos.z,
			hitNormal.x, hitNormal.y, hitNormal.z
		);

		// 斜めの角に当たっても扱いやすいよう、法線を軸方向(±X/±Y/±Z)に丸める
		Math::Vector3 axisNormal = BlockGridManager::SnapNormalToAxis(hitNormal);
		
		// 面ハイライト更新
		UpadateFaceHighlight(hitObj, axisNormal);

		auto spGround = FindGround();
		Math::Vector3 newBaseCell;
		if (spGround)
		{
			newBaseCell = BlockGridManager::Instance().ResolvePlaceableCell(hitPos, axisNormal, *spGround);
		}
		else
		{
			Math::Vector3 targetCellPos = hitPos + axisNormal * (BlockGridManager::GridSize * 0.5f + 0.01f);
			newBaseCell = BlockGridManager::Instance().SnapToGrid(targetCellPos);
		}

		// 狙っているセル/方向が変わった時だけプレビューを作り直す(無駄な再生成を防ぐ)
		bool cellChanged = (newBaseCell - m_aimBaseCell).LengthSquared() > 0.01f;
		bool dirChanged = (axisNormal - m_aimDir).LengthSquared() > 0.01f;
		bool firstTime = !m_hasValidAim;

		m_aimBaseCell = newBaseCell;
		m_aimDir = axisNormal;
		m_hasValidAim = true;

		if (cellChanged || dirChanged || firstTime)
		{
			RebuildPreview();
		}
	}
	else if (m_hasValidAim)
	{
		// 何にも当たらない方向を向いたらプレビューを消す
		for (auto& block : m_previewBlocks) block->Expire();
		m_previewBlocks.clear();
		m_hasValidAim = false;
		ClearFaceHighlight();	// 何も狙えなくなったら面ハイライトも消す
	}
}

// ===================================================
// Adjusting中、毎フレーム呼ばれる：レティクル方向のレイが
// どのプレビューブロックに当たっているかを判定し、ハイライトを切り替える
// ===================================================
void Magicwand::UpdateHighlight(const Math::Vector3& muzzlePos, const Math::Matrix& parentMat)
{
	if (m_previewBlocks.empty()) return;

	Math::Vector3 dir = parentMat.Backward();
	Math::Vector3 halfExtent(BlockGridManager::GridSize * 0.5f, BlockGridManager::GridSize * 0.5f, BlockGridManager::GridSize * 0.5f);

	// 常に「起点のマス」＝最初のプレビューブロックの面を判定対象にする
	auto& baseBlock = m_previewBlocks.front();

	float dist = 0.0f;
	Math::Vector3 faceNormal;
	bool isHit = IntersectRayAABB(muzzlePos, dir, baseBlock->GetPos(), halfExtent, 1000.0f, &dist, &faceNormal);

	if (!isHit)
	{
		baseBlock->SetHighlightFace(false);
		return;
	}

	// 狙っている面をハイライト
	baseBlock->SetHighlightFace(true, faceNormal);

	// 狙っている面の方向が今の伸びる方向と違うなら、方向を切り替えてスタックを作り直す
	if ((faceNormal - m_aimDir).LengthSquared() > 0.01f)
	{
		m_aimDir = faceNormal;
		RebuildPreview(); // ← ここでm_previewBlocksが全部新しく作り直される

		// 作り直した直後のbaseBlockは別インスタンスになっているので、ハイライトを付け直す
		if (!m_previewBlocks.empty())
		{
			m_previewBlocks.front()->SetHighlightFace(true, faceNormal);
		}
	}
}

// ===================================================
// 現在の段数(m_stackCount)・方向(m_aimDir)を元に、
// プレビュー用の半透明ブロックを作り直す
// (呼ばれるたびに古いプレビューは全部消してから作り直すシンプルな実装)
// ===================================================
void Magicwand::RebuildPreview()
{
	// 前回分のプレビューをすべて消す
	for (auto& block : m_previewBlocks) block->Expire();
	m_previewBlocks.clear();

	auto spGround = FindGround();
	if (!spGround) return;			//Groundが見つからない異常時は何も表示しない

	// 実際に何段まで置けるかを衝突判定込みで計算する
	auto positions = BlockGridManager::Instance().TryStack(m_aimBaseCell, m_aimDir, m_stackCount, *spGround);

	for (size_t i = 0; i < positions.size(); ++i)
	{
		auto preview = std::make_shared<NormalBlock>();
		preview->Init(positions[i]);
		preview->SetPreview(true);   // 見た目だけの「見本」扱いにする(当たり判定OFFなど)
		
		if (i > 0)
		{
			// 基点から伸びた分だけ光らせる(伸びた範囲を示す)
			preview->SetHighlight(true);
		}
		// i == 0(基点)は何もしない → 今まで通りの半透明プレビューのまま

		SceneManager::Instance().AddObject(preview);
		m_previewBlocks.push_back(preview); 
	}
}

// ===================================================
// 確定操作(左クリックを離した瞬間)：ブロックはまだ生成せず、その場所に向けて弾を発射する
// ===================================================
void Magicwand::ConfirmStack(const Math::Vector3& muzzlePos, const Math::Matrix& parentMat)
{
	// ① Expireで消す前に、プレビューブロックの座標だけ先に保存しておく
	std::vector<Math::Vector3> previewPositions;
	previewPositions.reserve(m_previewBlocks.size());
	for (auto& block : m_previewBlocks)
	{
		previewPositions.push_back(block->GetPos());
	}

	if (!m_hasValidAim)
	{
		// 狙いが無効なまま呼ばれた場合のみ、ここで消して抜ける
		for (auto& block : m_previewBlocks)block->Expire();
		m_previewBlocks.clear();
		m_state = WandState::Idle;
		return;
	}

	// 生成用の情報(確定済みのスタック起点・方向・段数)保持
	Math::Vector3 confirmedBaseCell = m_aimBaseCell;
	Math::Vector3 confirmedDir		= m_aimDir;
	int confirmedCount				= m_stackCount;

	// ② 今のカメラ向きで改めてレイキャストする(UpdateAimModeと同じロジック)
	KdCollider::RayInfo rayInfo;
	rayInfo.m_pos	= muzzlePos;
	rayInfo.m_dir   = parentMat.Backward();
	rayInfo.m_range = 1000.0f;
	rayInfo.m_type  = KdCollider::TypeGround | KdCollider::TypeBump;

	std::list <KdCollider::CollisionResult> resultList;
	for (auto& obj : SceneManager::Instance().GetObjList())
	{
		obj->Intersects(rayInfo, &resultList);
	}

	bool  isHit		 = false;
	float minDistSqr = FLT_MAX;
	Math::Vector3 realHitPos	= Math::Vector3::Zero;
	Math::Vector3 realHitNormal = Math::Vector3::Up;

	for (auto& ret : resultList)
	{
		float distSqr = (ret.m_hitPos - muzzlePos).LengthSquared();
		if (distSqr < minDistSqr)
		{
			minDistSqr    = distSqr;
			realHitPos    = ret.m_hitPos;
			realHitNormal = ret.m_hitNDir;
			isHit = true;
		}
	}

	// 何にも当たらなかった場合は、レイの最大距離まで飛んだ先を仮の着弾点にする
	Math::Vector3 realTargetPos = isHit ? realHitPos : (muzzlePos + rayInfo.m_dir * rayInfo.m_range);

	// ③ 今の照準の着弾点が、確定済みプレビュー(緑ブロック)のどれかと重なっているか判定
	//    (地面のヒット点との座標比較ではなく、レイ×箱の交差判定で直接調べる)
	Math::Vector3 halfExtent(BlockGridManager::GridSize * 0.5f, BlockGridManager::GridSize * 0.5f, BlockGridManager::GridSize * 0.5f);
	
	// 何かに当たった位置までしか判定しない(壁の向こう側のブロックを誤検出しないため)
	float maxCheckDist = isHit ? sqrtf(minDistSqr) + 0.1f : rayInfo.m_range;

	bool isTargetHit = false;
	float closestHitDistance = FLT_MAX;	// 命中したブロックの中で一番手前の距離を覚えておく

	for (auto& pos : previewPositions)
	{
		float hitDistance = 0.0f;
		if (IntersectRayAABB(muzzlePos, rayInfo.m_dir, pos, halfExtent, maxCheckDist, &hitDistance))
		{
			isTargetHit = true;
			closestHitDistance = std::min(closestHitDistance, hitDistance);	 // 複数ブロックに当たりうるので一番近い方を採用
		}
	}

	// 命中していれば「ブロックに実際に当たった座標」を、していなければ従来通りrealTargetPosを使う
	Math::Vector3 bulletTargetPos = isTargetHit
		? (muzzlePos + rayInfo.m_dir * closestHitDistance)
		: realTargetPos;

	auto bulletObj = std::make_shared<Bullet>();

	// weak_ptrで持つことで、万が一Magicwand側が破棄されてもコールバック側で安全に弾ける
	std::weak_ptr<Magicwand> weakSelf =
		std::static_pointer_cast<Magicwand>(shared_from_this());

	if (isTargetHit)
	{
		// 命中：生成し、この時初めてプレビューを消してIdleに戻す
		// (m_previewBlocksはコールバック内でself経由で触るため、ここではコピーを渡さない)	
		bulletObj->Init(muzzlePos, bulletTargetPos, realHitNormal,
			[weakSelf, confirmedBaseCell, confirmedDir, confirmedCount]
			(const Math::Vector3&, const Math::Vector3&)
			{
				if (auto self = weakSelf.lock())
				{
					for (auto& block : self->m_previewBlocks)block->Expire();
					self->m_previewBlocks.clear();

					self->GenerateStackAt(confirmedBaseCell, confirmedDir, confirmedCount);
				
					self->m_hasValidAim = false;
					self->m_state = WandState::Idle;
				}
			});
	}
	else
	{
	    // ハズレ：プレビューには一切触らない。状態もAdjustingのまま維持する
	    // (右クリックでの明示的なキャンセル、または再度の左クリックでの命中を待つ)
		bulletObj->Init(muzzlePos, bulletTargetPos, realHitNormal,
			[](const Math::Vector3&, const Math::Vector3&) {/* 何もしない */});
	}

	SceneManager::Instance().AddObject(bulletObj);

	StartSwingAnim();
}

// ===================================================
// 弾が着弾した瞬間に呼ばれる：実際にブロックを生成する
// ===================================================
void Magicwand::GenerateStackAt(const Math::Vector3& baseCell, const Math::Vector3& dir, int stackCount)
{
	auto spGround = FindGround();
	if (!spGround) return;

	// 弾が飛んでいる間に状況が変わっている可能性があるため、着弾した瞬間にもう一度計算し直す
	auto positions = BlockGridManager::Instance().TryStack(baseCell, dir, stackCount, *spGround);
	if (positions.empty()) return;

	// 段数(生成される実際のブロック数)ぶんだけ消費する
	int needCasts = (int)positions.size();
	if (!MagicManager::Instance().TryConsumeCast(needCasts))
	{
		return;
	}

	// 1段ごとにアニメーション開始をずらし、根元から順に生えて見せる
	constexpr int staggerFrames = 6;
	std::vector<std::weak_ptr<NormalBlock>> newStack;	// このスタックの一員を記録するリスト

	int stackId = m_nextStackId++;

	for (size_t i = 0; i < positions.size(); ++i)
	{
		auto block = std::make_shared<NormalBlock>();
		block->Init(positions[i]);
		block->StartEmerge((int)i * staggerFrames, dir);
		block->SetStackInfo(stackId, (int)i);

		SceneManager::Instance().AddObject(block);

		// 見た目が完成する前でも、別の弾が同じマスに重ねて生成されないよう先に登録しておく
		BlockGridManager::Instance().Register(positions[i]);

		newStack.push_back(block);
	}

	m_generatedStacks.push_back(newStack);   // スタックの履歴に追加
}

// ===================================================
// キャンセル操作：何も生成せず、プレビューだけ消して通常状態に戻る
// ===================================================
void Magicwand::CancelAim()
{
	for (auto& block : m_previewBlocks)
	{
		block->Expire();
	}
	m_previewBlocks.clear();

	m_state = WandState::Idle;
}

void Magicwand::DismissStack(const std::vector<std::weak_ptr<NormalBlock>>& stack)
{
	constexpr int staggerFrames = 6;
	int delay = 0;

	for (auto it = stack.rbegin(); it != stack.rend(); ++it)
	{
		if (auto block = it->lock())
		{
			BlockGridManager::Instance().Unregister(block->GetPos());

			block->StartDismiss(delay);
			delay += staggerFrames;
		}
	}

}

void Magicwand::UpadateFaceHighlight(const std::shared_ptr<KdGameObject>& hitObj, const Math::Vector3& axisNormal)
{
	auto hitBlock = std::dynamic_pointer_cast<NormalBlock>(hitObj);
	auto prevBlock = m_wpFaceHighlightBlock.lock();

	if (prevBlock == hitBlock)
	{
		// 同じブロックの同じ面を狙い続けている場合でも、法線だけ更新しておく
		if (hitBlock)hitBlock->SetHighlightFace(true, axisNormal);
		return;
	}

	if (prevBlock)prevBlock->SetHighlightFace(false);
	if (hitBlock) hitBlock->SetHighlightFace(true, axisNormal);

	// hitObjがブロックでなければ自動的にリセットされる
	m_wpFaceHighlightBlock = hitBlock;
}

void Magicwand::ClearFaceHighlight()
{
	if (auto prev = m_wpFaceHighlightBlock.lock())
	{
		prev->SetHighlightFace(false);
	}
	m_wpFaceHighlightBlock.reset();
}
