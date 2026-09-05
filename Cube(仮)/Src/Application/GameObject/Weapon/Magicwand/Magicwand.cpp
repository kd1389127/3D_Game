#include "Magicwand.h"

#include "../../../main.h"                     
#include "../../../Scene/SceneManager.h"
#include "../../Bullet/Bullet.h"
#include "../../Character/Player/Player.h"
#include "../../Block/BlockGridManager.h"
#include "../../Block/NormalBlock/NormalBlock.h" 
#include "../../Map/Ground/Ground.h"

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

	// ===================================================
	// 状態(m_state)によって処理を分岐する
	// ===================================================
	switch (m_state)
	{
		// ---------------------------------------------------
		// 通常状態：左クリックで弾を発射する
		// ---------------------------------------------------
	case WandState::Idle:
	{
		// 左クリックの「押した瞬間」だけ発射する(押しっぱなし連射防止)
		if (GetAsyncKeyState(VK_LBUTTON) & 0x8000)
		{
			if (!m_mouseDownFlg)
			{
				m_mouseDownFlg = true;

				// ブロックを持ち上げている間は発射できない(誤操作防止)
				if (!isCarrying)
				{
					EnterAimMode();
					// 押した瞬間から即座にプレビューが出るよう、同じフレームで1回更新しておく
					UpdateAimMode(muzzlePos, parentMat);
				}
			}
		}
		else
		{
			m_mouseDownFlg = false;
		}
		break;
	}

	// ---------------------------------------------------
	// エイムモード：長押し中、レイを飛ばし続けてプレビューを追従させる
	// ---------------------------------------------------
	case WandState::Aiming:
		UpdateAimMode(muzzlePos, parentMat);
		break;
	}

	// 基底クラス(WeaponBase)の更新処理を呼んで、ワールド行列などを確定させる
	WeaponBase::Update();
}

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

void Magicwand::UpdateAimMode(const Math::Vector3& muzzlePos, const Math::Matrix& parentMat)
{
	// ----- 右クリックでキャンセル -----
	if (GetAsyncKeyState(VK_RBUTTON) & 0x8000)
	{
		CancelAim();
		return;
	}

	// ----- 左クリックを離したら確定 -----
	if (!(GetAsyncKeyState(VK_LBUTTON) & 0x8000))
	{
		m_mouseDownFlg = false;
		ConfirmStack(muzzlePos);
		return;
	}

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

	// 当たった候補の中から、一番近いもの(最初に当たるもの)を選ぶ
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
		for (auto& block : m_previewBlocks) block->Expire();
		m_previewBlocks.clear();
		m_hasValidAim = false;
	}

	// ----- ホイールで段数変更 -----
	int wheelValue = Application::Instance().GetMouseWheelValue();

	if (wheelValue != 0)
	{
		int newCount = std::clamp(m_stackCount + (wheelValue > 0 ? 1 : -1), 1, m_maxStackCount);

		if (newCount != m_stackCount)
		{
			m_stackCount = newCount;
			if (m_hasValidAim)
			{
				RebuildPreview();
			}
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

	auto positions = BlockGridManager::Instance().TryStack(m_aimBaseCell, m_aimDir, m_stackCount, *spGround);

	for (auto& pos : positions)
	{
		auto preview = std::make_shared<NormalBlock>();
		preview->Init(pos);
		preview->SetPreview(true); // 見た目だけの「見本」扱いにする(当たり判定OFFなど)

		SceneManager::Instance().AddObject(preview);
		m_previewBlocks.push_back(preview); // 次にRebuild/Confirm/Cancelする時に消せるよう保持
	}
}

void Magicwand::ConfirmStack(const Math::Vector3& muzzlePos)
{
	for (auto& block : m_previewBlocks) block->Expire();
	m_previewBlocks.clear();

	if (!m_hasValidAim)
	{
		m_state = WandState::Idle;
		return;
	}

	// ★確定した狙い(位置・方向・段数)を値としてコピーしておく
	//   (弾が届くまでの間にm_aimBaseCellなどが次のエイムで上書きされても影響を受けないようにするため)
	Math::Vector3 confirmedBaseCell = m_aimBaseCell;
	Math::Vector3 confirmedDir = m_aimDir;
	int confirmedCount = m_stackCount;

	auto bullet = std::make_shared<Bullet>();

	std::weak_ptr<Magicwand> weakSelf =
		std::static_pointer_cast<Magicwand>(shared_from_this());

	bullet->Init(muzzlePos, confirmedBaseCell, confirmedDir,
		[weakSelf, confirmedBaseCell, confirmedDir, confirmedCount](const Math::Vector3&, const Math::Vector3&)
		{
			if (auto self = weakSelf.lock())
			{
				self->GenerateStackAt(confirmedBaseCell, confirmedDir, confirmedCount);
			}
		});

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

	constexpr int staggerFrames = 6;

	for (size_t i = 0; i < positions.size(); ++i)
	{
		auto block = std::make_shared<NormalBlock>();
		block->Init(positions[i]);
		block->StartEmerge((int)i * staggerFrames, dir);

		SceneManager::Instance().AddObject(block);

		BlockGridManager::Instance().Register(positions[i]);
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