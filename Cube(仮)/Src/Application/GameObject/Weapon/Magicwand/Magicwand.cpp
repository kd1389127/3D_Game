#include "Magicwand.h"

#include "../../../main.h"                     
#include "../../../Scene/SceneManager.h"
#include "../../Bullet/Bullet.h"
#include "../../Character/Player/Player.h"
#include "../../Block/BlockGridManager.h"
#include "../../Block/NormalBlock/NormalBlock.h" 
#include "../../Map/Ground/Ground.h"

namespace
{
	// ===================================================
	// レイ(origin, dir)が、center±halfExtentsのAABBに当たるか判定する(スラブ法)
	// 当たった場合、出会うまでの距離(tOut)を返す
	// ===================================================
	bool IntersectRayAABB(const Math::Vector3& origin, const Math::Vector3& dir,
		const Math::Vector3& center, const Math::Vector3& halfExtents, float maxDist, float* tOut = nullptr)
	{
		Math::Vector3 boxMin = center - halfExtents;
		Math::Vector3 boxMax = center + halfExtents;

		float tMin = 0.0f;
		float tMax = maxDist;

		const float* originArr = &origin.x;
		const float* dirArr = &dir.x;
		const float* boxMinArr = &boxMin.x;
		const float* boxMaxArr = &boxMax.x;

		// X/Y/Z の3軸それぞれについて、レイが箱の範囲内にいる区間[tMin, tMax]を絞り込んでいく
		for (int axis = 0; axis < 3; ++axis)
		{
			float o = originArr[axis];
			float d = dirArr[axis];
			float mn = boxMinArr[axis];
			float mx = boxMaxArr[axis];

			if (fabsf(d) < 1e-6f)
			{
				// この軸方向にはほぼ動かないレイ：原点がすでに範囲外なら絶対当たらない
				if (o < mn || o > mx) return false;
			}
			else
			{
				float t1 = (mn - o) / d;
				float t2 = (mx - o) / d;
				if (t1 > t2) std::swap(t1, t2);

				tMin = std::max(tMin, t1);
				tMax = std::min(tMax, t2);

				if (tMin > tMax) return false; // 区間が潰れた＝当たらない
			}
		}

		if (tOut) *tOut = tMin;
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

		// 親(プレイヤー)から見た「杖本体」の相対位置(手元に構えている位置)
		m_localMat = Math::Matrix::CreateTranslation(0.2f, -0.55f, 0.4f);

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
	if (spParent)
	{
		auto player = std::dynamic_pointer_cast<const Player>(spParent);
		if (player && player->IsCarryingBlock())
		{
			isCarrying = true;
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

	// グリッドの表示
	//if (m_pDebugWire)
	//{
	//	Math::Vector3 playerPos = parentMat.Translation();
	//	BlockGridManager::Instance().DrawDebugGrid(*m_pDebugWire, playerPos, 40.0f); // 半径40くらい表示
	//}

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
			// ブロックを持ち上げている間は発射できない(誤操作防止)
			if (rightPressed && !isCarrying)
			{
				EnterAimMode();
				// 押した瞬間から即座にプレビューが出るよう、同じフレームで1回更新しておく
				UpdateAimMode(muzzlePos, parentMat);
			}
			else if (leftPressed && !isCarrying)
			{
				// 左クリック単発：エイムなしで即座に1個だけ生成する
				SingleShot(muzzlePos, parentMat);
			}
			break;
		}

		// ---------------------------------------------------
		// エイムモード：長押し中、レイを飛ばし続けてプレビューを追従させる
		// ---------------------------------------------------
		case WandState::Aiming:
		{
			UpdateAimMode(muzzlePos, parentMat);

			// 右クリックを離したら、狙っている位置で固定して調整モードへ
			if (rightReleased)
			{
				if (m_hasValidAim)
				{
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
				int newCount = std::clamp(m_stackCount + (wheelValue > 0 ? 1 : -1), 1, m_maxStackCount);

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

	// 基底クラス(WeaponBase)の更新処理を呼んで、ワールド行列などを確定させる
	WeaponBase::Update();
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
	std::weak_ptr<Magicwand> weakSelf = std::static_pointer_cast<Magicwand>(shared_from_this());

	if (isHit)
	{
		// 斜めの角に当たっても扱いやすいよう、法線を軸方向(±X/±Y/±Z)に丸める
		Math::Vector3 axisNormal = BlockGridManager::SnapNormalToAxis(hitNormal);
		auto spGround = FindGround();

		Math::Vector3 baseCell;
		if (spGround)
		{
			baseCell = BlockGridManager::Instance().ResolvePlaceableCell(hitPos, axisNormal, *spGround);
		}
		else
		{
			Math::Vector3 targetCellPos = hitPos + axisNormal * (BlockGridManager::GridSize * 0.5f + 0.01f);
			baseCell = BlockGridManager::Instance().SnapToGrid(targetCellPos);
		}

		// 着弾した瞬間、1個だけ(stackCount=1)生成する
		bullet->Init(muzzlePos, targetPos, hitNormal,
			[weakSelf, baseCell, axisNormal](const Math::Vector3&, const Math::Vector3&)
			{
				if (auto self = weakSelf.lock())
				{
					self->GenerateStackAt(baseCell, axisNormal, 1);
				}
			});
	}
	else
	{
		// 何にも当たらなかった：弾は飛ぶが何も生成しない
		bullet->Init(muzzlePos, targetPos, hitNormal,
			[](const Math::Vector3&, const Math::Vector3&) {/* 何もしない */});
	}

	SceneManager::Instance().AddObject(bullet);

}

// エイムモード開始：段数・狙い情報をリセットする
void Magicwand::EnterAimMode()
{
	m_state = WandState::Aiming;
	m_stackCount = 1;
	m_hasValidAim = false;
	m_aimBaseCell = Math::Vector3::Zero;
	m_aimDir = Math::Vector3::Up;
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

	// マップ上の全オブジェクトに対してレイ判定を行う
	for (auto& obj : SceneManager::Instance().GetObjList())
	{
		obj->Intersects(rayInfo, &resultList);
	}

	// 当たった候補の中から一番近いものを選ぶ
	bool isHit = false;
	float minDistSqr = FLT_MAX;
	Math::Vector3 hitPos = Math::Vector3::Zero;
	Math::Vector3 hitNormal = Math::Vector3::Up;

	for (auto& ret : resultList)
	{
		float distSqr = (ret.m_hitPos - muzzlePos).LengthSquared(); // 平方根計算を省いて軽量化

		if (distSqr < minDistSqr)
		{
			minDistSqr = distSqr;
			hitPos = ret.m_hitPos;
			hitNormal = ret.m_hitNDir;
			isHit = true;
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

	for (auto& pos : positions)
	{
		auto preview = std::make_shared<NormalBlock>();
		preview->Init(pos);
		preview->SetPreview(true); // 見た目だけの「見本」扱いにする(当たり判定OFFなど)

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

	// ここではまだExpireしない。弾の着弾コールバックまで持ち越すため、
	// m_previewBlocksの中身をローカル変数に移し替えて、メンバ自体は空にしておく
	std::vector<std::shared_ptr<NormalBlock>> previewBlocksToExpire = std::move(m_previewBlocks);
	m_previewBlocks.clear();

	if (!m_hasValidAim)
	{
		// 狙いが無効なまま呼ばれた場合は、ここで消しておく(通常はほぼ空のはず)
		for (auto& block : previewBlocksToExpire)block->Expire();
		m_state = WandState::Idle;
		return;
	}

	// 生成用の情報(確定済みのスタック起点・方向・段数)保持
	Math::Vector3 confirmedBaseCell = m_aimBaseCell;
	Math::Vector3 confirmedDir = m_aimDir;
	int confirmedCount = m_stackCount;

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

	auto bullet = std::make_shared<Bullet>();

	// weak_ptrで持つことで、万が一Magicwand側が破棄されてもコールバック側で安全に弾ける
	std::weak_ptr<Magicwand> weakSelf =
		std::static_pointer_cast<Magicwand>(shared_from_this());

	if (isTargetHit)
	{
		// previewBlocksToExpireをコールバックに持ち越し、着弾した瞬間に消す
		bullet->Init(muzzlePos, bulletTargetPos, realHitNormal,
			[weakSelf, confirmedBaseCell, confirmedDir, confirmedCount,previewBlocksToExpire]
			(const Math::Vector3&, const Math::Vector3&)
			{
				for (auto& block : previewBlocksToExpire)block->Expire();

				if (auto self = weakSelf.lock())
				{
					self->GenerateStackAt(confirmedBaseCell, confirmedDir, confirmedCount);
				}
			});
	}
	else
	{
		// ハズレの場合も、弾が着弾した(何にも当たらなかった)瞬間にプレビューを消す
		bullet->Init(muzzlePos, bulletTargetPos, realHitNormal,
			[previewBlocksToExpire](const Math::Vector3&, const Math::Vector3&) 
			{
				for (auto& block : previewBlocksToExpire)block->Expire();
			});
	}

	SceneManager::Instance().AddObject(bullet);

	m_hasValidAim = false;
	m_state = WandState::Idle;
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

	// 1段ごとにアニメーション開始をずらし、根元から順に生えて見せる
	constexpr int staggerFrames = 6;

	std::vector<std::weak_ptr<NormalBlock>> newStack;	// このスタックの一員を記録するリスト

	for (size_t i = 0; i < positions.size(); ++i)
	{
		auto block = std::make_shared<NormalBlock>();
		block->Init(positions[i]);
		block->StartEmerge((int)i * staggerFrames, dir);

		SceneManager::Instance().AddObject(block);

		// 見た目が完成する前でも、別の弾が同じマスに重ねて生成されないよう先に登録しておく
		BlockGridManager::Instance().Register(positions[i]);

		newStack.push_back(block);
	}

	m_generatedStacks.push_back(newStack);   // スタックの履歴に追加

	// スタック数が上限を超えたら、一番古いスタックをまとめて消す
	if((int)m_generatedStacks.size() > m_maxAliveStacks)
	{
		auto oldest = m_generatedStacks.front();
		m_generatedStacks.pop_front();
		DismissStack(oldest);
	}

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
