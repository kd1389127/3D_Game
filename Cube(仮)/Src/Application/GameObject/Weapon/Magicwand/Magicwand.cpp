#include "Magicwand.h"

#include "../../../Scene/SceneManager.h"
#include "../../Bullet/Bullet.h"
#include "../../Character/Player/Player.h"

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

	// 親がブロックを持ち上げ中かどうかを確認する
	bool isCarrying = false;
	if (spParent)
	{
		// 親をPlayer型にキャストできれば、ブロックを持っているか聞く
		auto player = std::dynamic_pointer_cast<const Player>(spParent);
		if (player && player->IsCarryingBlock())
		{
			isCarrying = true;
		}
	}

	// 左クリックで発射(ただし押しっぱなし対策として「押した瞬間」だけ反応する)
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

	// 親(プレイヤー)のワールド行列を取得(なければ単位行列＝原点扱い)
	Math::Matrix parentMat = Math::Matrix::Identity;
	if (spParent)
	{
		parentMat = spParent->GetMatrix();
	}

	// 銃口(発射位置)のワールド座標を計算する
	Math::Vector3 muzzlePos = (m_localMuzzleMat * parentMat).Translation();
	//m_pDebugWire->AddDebugSphere(muzzlePos, 0.05f,kRedColor); // デバッグ表示用(コメントアウト中)

	// ----- 発射フラグが立っていたら、実際に弾を撃つ処理 -----
	if (m_shotFlg)
	{
		// 銃口からプレイヤーの向いている方向へ、着弾点を調べるためのレイを飛ばす
		KdCollider::RayInfo rayInfo;
		rayInfo.m_pos = muzzlePos;
		rayInfo.m_dir = parentMat.Backward(); // プレイヤーの前方向
		rayInfo.m_range = 1000.0f; // 十分に遠くまで判定する
		rayInfo.m_type = KdCollider::TypeGround | KdCollider::TypeBump;

		std::list <KdCollider::CollisionResult> resultList;

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
		// (弾自体は見た目上の演出。実際の当たり判定はここで先に計算済み)
		if (isHit)
		{
			auto bullet = std::make_shared<Bullet>();
			bullet->Init(muzzlePos, hitPos, hitNormal);

			SceneManager::Instance().AddObject(bullet);
		}

		// 発射処理が終わったのでフラグを戻す(次のクリックでまた撃てるように)
		m_shotFlg = false;
		m_rayBulletFlg = false;
	}

	// 基底クラス(WeaponBase)の更新処理を呼んで、ワールド行列などを確定させる
	WeaponBase::Update();
}

// 発射フラグを立てるだけの関数(実際の発射処理はUpdate内で行われる)
void Magicwand::ShotBullet(const bool _rayFlg)
{
	m_shotFlg = true;
	m_rayBulletFlg = _rayFlg; // レイとして扱うか、実体のある弾として扱うかのフラグ(現状未使用箇所あり)
}