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
					ShotBullet();
				}
			}
		}
		else
		{
			m_mouseDownFlg = false;
		}

		// ----- 発射フラグが立っていたら、実際に弾を撃つ処理 -----
		if (m_shotFlg)
		{
			// 銃口からプレイヤーの向いている方向へ、着弾点を調べるためのレイを飛ばす
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

			// 何かに当たっていたら、その着弾点に向かう弾オブジェクトを生成する
			if (isHit)
			{
				KdDebugGUI::Instance().AddLog(
					"[Wand] hitPos: X=%.2f Y=%.2f Z=%.2f / hitNormal: X=%.2f Y=%.2f Z=%.2f\n",
					hitPos.x, hitPos.y, hitPos.z,
					hitNormal.x, hitNormal.y, hitNormal.z
				);
				auto bullet = std::make_shared<Bullet>();

				// this(Magicwand自身)をweak_ptrにして、Bulletのコールバックに渡す
				// → weak_ptrにするのは、Bulletが生きている間にMagicwand側が破棄されても
				//   コールバック実行時にlock()が失敗するだけで安全に済むようにするため
				std::weak_ptr<Magicwand> weakSelf =
					std::static_pointer_cast<Magicwand>(shared_from_this());

				// ★変更点：着弾したらBulletが直接ブロックを生成するのではなく、
				//   Magicwand::EnterAdjustMode を呼んで「段数調整モード」に入るようにする
				bullet->Init(muzzlePos, hitPos, hitNormal,
					[weakSelf](const Math::Vector3& pos, const Math::Vector3& normal)
					{
						if (auto self = weakSelf.lock())
						{
							self->EnterAdjustMode(pos, normal);
						}
					});

				SceneManager::Instance().AddObject(bullet);
			}

			// 発射処理が終わったのでフラグを戻す(次のクリックでまた撃てるように)
			m_shotFlg = false;
			m_rayBulletFlg = false;
		}
		break;
	}

	// ---------------------------------------------------
	// 調整モード：ホイールで段数を決め、クリックで確定する
	// ---------------------------------------------------
	case WandState::Adjusting:
		UpdateAdjustMode();
		break;
	}

	// 基底クラス(WeaponBase)の更新処理を呼んで、ワールド行列などを確定させる
	WeaponBase::Update();
}

// ===================================================
// 弾が着弾した時に呼ばれる：段数調整モードに入る準備をする
// hitPos    … 着弾したワールド座標(そのままだと壁の表面ぴったりの位置)
// axisNormal… 着弾した面の法線を軸方向(±X,±Y,±Zのどれか)に丸めたもの
// ===================================================
void Magicwand::EnterAdjustMode(const Math::Vector3& hitPos, const Math::Vector3& axisNormal)
{
	auto spGround = FindGround();

	if (spGround)
	{
		m_aimBaseCell = BlockGridManager::Instance().ResolvePlaceableCell(hitPos, axisNormal, *spGround);
	}
	else
	{
		Math::Vector3 targetCellPos = hitPos + axisNormal * (BlockGridManager::GridSize * 0.5f + 0.01f);
		m_aimBaseCell = BlockGridManager::Instance().SnapToGrid(targetCellPos);
	}

	m_aimDir = axisNormal;
	m_stackCount = 1;
	m_state = WandState::Adjusting;

	RebuildPreview();
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
// 調整モード中、毎フレーム呼ばれる入力処理
// ===================================================
void Magicwand::UpdateAdjustMode()
{
	// ----- ホイールで段数変更 -----
	int wheelValue = Application::Instance().GetMouseWheelValue();

	if (wheelValue != 0)
	{
		// 値の大きさ(何クリック分か)は使わず、符号(奥/手前)だけ見て1段ずつ増減させる
		int newCount = std::clamp(m_stackCount + (wheelValue > 0 ? 1 : -1), 1, m_maxStackCount);

		if (newCount != m_stackCount)
		{
			m_stackCount = newCount;
			RebuildPreview(); // 段数が変わったのでプレビューを作り直す
		}
	}

	// ----- 左クリックで確定 -----
	if (GetAsyncKeyState(VK_LBUTTON) & 0x8000)
	{
		if (!m_mouseDownFlg)
		{
			m_mouseDownFlg = true;
			ConfirmStack();
		}
	}
	else
	{
		m_mouseDownFlg = false;
	}

	// ----- 右クリックでキャンセル -----
	if (GetAsyncKeyState(VK_RBUTTON) & 0x8000)
	{
		if (!m_rightMouseDownFlg)
		{
			m_rightMouseDownFlg = true;
			CancelAdjustMode();
		}
	}
	else
	{
		m_rightMouseDownFlg = false;
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

// ===================================================
// 確定操作：プレビューを消し、実際のブロックを生成する
// ===================================================
void Magicwand::ConfirmStack()
{
	// プレビュー(見本)は役目を終えたので消す
	for (auto& block : m_previewBlocks) block->Expire();
	m_previewBlocks.clear();

	auto spGround = FindGround();
	if (!spGround)
	{
		m_state = WandState::Idle;
		return;
	}

	// 最終的な生成位置を、プレビューと同じロジックでもう一度計算する
	// (プレビュー表示後に別の要因でマスが埋まっている可能性もあるため、確定直前に取り直す)
	auto positions = BlockGridManager::Instance().TryStack(m_aimBaseCell, m_aimDir, m_stackCount,*spGround);

	constexpr int staggerFrames = 6; // 1段ごとにアニメーション開始をずらすフレーム数
	// (0段目→6f後→12f後…と遅れて出現させることで、
	//  根元から順に生えてくるように見せる)

	for (size_t i = 0; i < positions.size(); ++i)
	{
		auto block = std::make_shared<NormalBlock>();
		block->Init(positions[i]);

		// i段目ほど出現開始を遅らせ、狙った方向(m_aimDir)からせり出すよう指定
		block->StartEmerge((int)i * staggerFrames, m_aimDir);

		SceneManager::Instance().AddObject(block);

		// ★アニメーション中でもグリッド上は即座に「使用中」として登録しておく
		//   (見た目が出来上がる前でも、別の弾が同じマスに重ねて生成されるのを防ぐため)
		BlockGridManager::Instance().Register(positions[i]);
	}

	m_state = WandState::Idle; // 調整モード終了、通常状態へ戻る
}

// ===================================================
// キャンセル操作：何も生成せず、プレビューだけ消して通常状態に戻る
// ===================================================
void Magicwand::CancelAdjustMode()
{
	for (auto& block : m_previewBlocks)
	{
		block->Expire();
	}
	m_previewBlocks.clear();

	m_state = WandState::Idle;
}

// 発射フラグを立てるだけの関数(実際の発射処理はUpdate内で行われる)
void Magicwand::ShotBullet(const bool _rayFlg)
{
	m_shotFlg = true;
	m_rayBulletFlg = _rayFlg; // レイとして扱うか、実体のある弾として扱うかのフラグ(現状未使用箇所あり)
}