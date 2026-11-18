#pragma once
#include "../CameraBase.h"

class Magicwand;

class FPSCamera : public CameraBase
{
public:
	FPSCamera()							{}
	~FPSCamera()			override	{}

	void Init()				override;
	void PostUpdate()		override;

	// GameScene::Init側から、参照する杖を渡してもらう
	void SetMagicwand(const std::shared_ptr<Magicwand>& wand)
	{
		m_wpMagicwand = wand;
	}

private:

	std::weak_ptr<Magicwand> m_wpMagicwand;	// 参照カウンタを増やさない

	// ----- ADS(照準)用のFOV管理 -----
	static constexpr float m_defaultFov   = 60.0f; // 通常時のFOV
	static constexpr float m_adsFov		  = 40.0f; // エイム中(ADS)のFOV
	static constexpr float m_fovLerpSpeed = 0.15f; // 1フレームあたり目標値へ近づける割合(0～1)

	float m_currentFov = m_defaultFov;	// 現在描画に使っているFOV(毎フレーム目標値へ近づけていく)
};