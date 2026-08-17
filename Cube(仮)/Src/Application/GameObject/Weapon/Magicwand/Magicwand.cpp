#include "Magicwand.h"

#include "../../../Scene/SceneManager.h"
#include "../../Bullet/Bullet.h"

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

		// 親から武器本体へローカル行列 (相対位置)
		m_localMat = Math::Matrix::CreateTranslation(0.2f, -0.55f, 0.4f);

		// 親から銃口へのローカル行列(相対位置)
		// 「銃本体からへの相対位置」と「親から武器本体への相対位置」
		m_localMuzzleMat = Math::Matrix::CreateTranslation(0.2f, 0.7f, 0.7f);
		m_localMuzzleMat = m_localMuzzleMat * m_localMat;
	}
}


void Magicwand::Update()
{
	if (GetAsyncKeyState(VK_LBUTTON) & 0x8000)
	{
		if (!m_mouseDownFlg)
		{
			m_mouseDownFlg = true;
			ShotBullet();
		}
	}
	else
	{
		m_mouseDownFlg = false;
	}

	// 親(プレイヤー)の行列を取得
	Math::Matrix parentMat = Math::Matrix::Identity;
	const std::shared_ptr<const KdGameObject> spParent = m_wpParent.lock();
	if (spParent)
	{
		// 親の行列を取得
		parentMat = spParent->GetMatrix();
	}

	// 銃口位置をデバック表示
	Math::Vector3 muzzlePos = (m_localMuzzleMat * parentMat).Translation();
	m_pDebugWire->AddDebugSphere(muzzlePos, 0.05f,kRedColor);

	// 弾発射
	if (m_shotFlg)
	{
		
		// レイ判定用パラメーター
		KdCollider::RayInfo rayInfo;

		// レイの各パラメーターを設定
		rayInfo.m_pos = muzzlePos;
		rayInfo.m_dir = parentMat.Backward();
		rayInfo.m_range = 1000.0f;
		rayInfo.m_type = KdCollider::TypeGround | KdCollider::TypeBump;

		// 衝突情報リスト
		std::list <KdCollider::CollisionResult> resultList;

		// 作成したレイ情報でオブジェクトリストと当たり判定
		for (auto& obj : SceneManager::Instance().GetObjList())
		{
			obj->Intersects(rayInfo, &resultList);
		}
		
		// 衝突情報リストから一番近いオブジェクトを検出
		bool isHit = false;
		float maxOverLap = 0.0f;
		Math::Vector3 hitPos = Math::Vector3::Zero;
		Math::Vector3 hitNormal = Math::Vector3::Up;

		for (auto& ret : resultList)
		{
			// レイが当たった場合の貫通した長さが一番長いものを探す
			if (maxOverLap < ret.m_overlapDistance)
			{
				maxOverLap = ret.m_overlapDistance;
				hitPos = ret.m_hitPos;
				hitNormal = ret.m_hitNDir;
				isHit = true;
			}
		}

		// レイHit時
		if (isHit)
		{
			auto bullet = std::make_shared<Bullet>();
			bullet->Init(muzzlePos, hitPos,hitNormal);

			SceneManager::Instance().AddObject(bullet);
		}

		// 弾の発射が終わったらフラグを未発射に戻す
		m_shotFlg = false;
		m_rayBulletFlg = false;
	}

	// 基底クラスの更新処理(ワールド行列作成)
	WeaponBase::Update();
}

void Magicwand::ShotBullet(const bool _rayFlg)
{
	// 発射フラグON
	m_shotFlg = true;

	// レイ = 弾とするか
	m_rayBulletFlg = _rayFlg;
}
