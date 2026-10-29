#include "Goal.h"

void Goal::Init(const Math::Vector3& pos) 
{
	if (!m_spModel)
	{
		m_spModel = std::make_shared<KdModelWork>();
		// 仮モデル：とりあえず既存のモデルを流用してOK(例：NormalBlockのモデルなど)
		m_spModel->SetModelData("Asset/Models/Map/Goal/Goal.gltf");

		m_pCollider = std::make_unique<KdCollider>();
		// TypeEvent = イベント判定用(触れたら何か起きる、という用途)
		m_pCollider->RegisterCollisionShape("Goal", m_spModel, KdCollider::TypeEvent);
	}

	SetScale(18.0f);
	SetPos(pos);
}

void Goal::Update()
{
	const std::shared_ptr<KdGameObject> spTarget = m_wpTarget.lock();
	if (!spTarget) return;

	// ----- 調整用の値 -----
	const float kEnterRadius	= 40.0f;	// この距離に入ったら起動
	const float kExitRadius		= 48.0f;	// この距離より離れたら消し始める
	const float kMaxHeightDiff	= 30.0f;
	const float kFloorOffsetY	= 2.8f;		// 魔法陣の高さの調整
	const float kCircleSize		= 3.5f;		// 魔法陣の最大の大きさ
	const float kFadeInStep		= 1.0f / 20.0f;	// 約0.3秒で大きくなる
	const float kFadeOutStep	= 1.0f / 30.0f;	// 約0.5秒で小さくなって消える

	// ----- 近いかどうかの判定 -----
	const Math::Vector3 diff = spTarget->GetPos() - GetPos();
	const float distXZ = sqrtf(diff.x * diff.x + diff.z * diff.z);
	const bool sameLevel = fabsf(diff.y) <= kMaxHeightDiff;

	const bool wasNear = m_isNear;	// 判定を変える前の状態を覚えておく

	if (m_isNear)
	{
		if (distXZ > kExitRadius || !sameLevel) m_isNear = false;
	}
	else
	{
		if (distXZ < kEnterRadius && sameLevel) m_isNear = true;
	}

	// 近づいた瞬間だけ鳴らす(前は遠くて、今回近くなった時)
	if (m_isNear && !wasNear)
	{
		auto se = KdAudioManager::Instance().Play("Asset/Sounds/SE/Gimmick/Goal.wav", false);
		if (se)
		{
			se->SetVolume(0.15f);	// 高くすると聞こえやすくなるので調整
			se->SetPitch(1.0f);		// 近づいた時は高め。
		}
	}

	// ----- 出現の進み具合を、近いかどうかに向けて少しずつ動かす -----
	if (m_isNear) m_fadeRate = std::min(1.0f, m_fadeRate + kFadeInStep);
	else          m_fadeRate = std::max(0.0f, m_fadeRate - kFadeOutStep);

	// 動きをなめらかにする(始まりと終わりがゆっくりになる)
	const float eased = m_fadeRate * m_fadeRate * (3.0f - 2.0f * m_fadeRate);
	const float scale = kCircleSize * eased;

	// ----- エフェクトの再生・拡大縮小・停止 -----
	const std::shared_ptr<KdEffekseerObject> spEffect = m_wpEffect.lock();
	const bool isPlaying = spEffect && spEffect->IsPlaying();

	if (isPlaying)
	{
		if (m_fadeRate <= 0.0f)
		{
			spEffect->StopEffect();		// 小さくなりきったら止める
		}
		else
		{
			spEffect->SetScale(scale);	// 毎フレーム大きさを合わせる
		}
	}
	else if (m_isNear)
	{
		// 近くにいるのに再生されていない(最初、または終わった)時は、再生する
		m_wpEffect = KdEffekseerManager::GetInstance().Play(
			"Goal.efkefc", GetPos() + Math::Vector3(0.0f, kFloorOffsetY, 0.0f), scale);
	}

	// ----- 光の柱(遠くからゴールの場所が分かる目印) -----
	// 魔法陣が出てくるほど薄くなり、魔法陣と入れ替わるように消える(eased は魔法陣と同じ進み具合)
	const float pillarAlpha = 1.0f - eased;		// 1=くっきり 0=消える
	
	std::shared_ptr<KdEffekseerObject> spPillar = m_wpPillar.lock();
	const bool isPillarPlaying = spPillar && spPillar->IsPlaying();

	if (pillarAlpha > 0.0f)
	{
		if (!isPillarPlaying)
		{
			// 遠くにいて再生されていない時(最初、または消えきった後)は、再生する
			const Math::Vector3 pillarOffset(kPillarOffsetX, kPillarOffsetY, kPillarOffsetZ);
			m_wpPillar = KdEffekseerManager::GetInstance().Play(
				"Landmark.efkefc", GetPos() + pillarOffset, kPillarWidth);
			spPillar = m_wpPillar.lock();
			spPillar->SetScale(Math::Vector3(kPillarWidth, kPillarHeight, kPillarWidth));	// 太さと高さ
		}
		spPillar->SetAlpha(pillarAlpha);
	}
	else if (isPillarPlaying)
	{
		spPillar->StopEffect();
	}

}

void Goal::DrawUnLit()
{
	if (!m_spModel) return;

	KdShaderManager::Instance().m_StandardShader.DrawModel(*m_spModel, m_mWorld);
}