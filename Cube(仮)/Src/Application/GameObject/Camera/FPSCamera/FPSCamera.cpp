#include "FPSCamera.h"

#include "../../Weapon/Magicwand/Magicwand.h"

void FPSCamera::Init()
{
	// 親クラスの初期化呼び出し
	CameraBase::Init();
}

void FPSCamera::PostUpdate()
{
	// ターゲットの行列(有効な場合利用する)
	Math::Matrix								_targetMat = Math::Matrix::Identity;
	const std::shared_ptr<const KdGameObject>	_spTarget = m_wpTarget.lock();
	if (_spTarget)
	{
		_targetMat = _spTarget->GetMatrix();
	}

	// カメラの回転
	m_mWorld = _targetMat;

	// ----- ADS風のFOV変更処理 -----
	// エイム中(Magicwandが狙い中/調整中)なら狭いFOVへ、そうでなければ通常FOVへ、毎フレーム少しずつ近づける
	bool isAiming = false;
	if (auto spWand = m_wpMagicwand.lock())
	{
		// ズーム処理
		isAiming = spWand->ShouldZoom();
	}

	float targetFov = isAiming ? m_adsFov : m_defaultFov;
	m_currentFov += (targetFov - m_currentFov) * m_fovLerpSpeed;

	if (m_spCamera)
	{
		m_spCamera->SetProjectionMatrix(m_currentFov);
	}
}
