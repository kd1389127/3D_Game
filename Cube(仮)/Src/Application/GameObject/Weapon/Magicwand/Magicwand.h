#pragma once
#include "../WeaponBase.h"

class Magicwand : public WeaponBase
{
public:

	Magicwand()		   {}
	~Magicwand() override {}

	void Init()		override;
	void Update()	override;
	
	// 弾発射関数
	void ShotBullet(const bool _rayFlg = false) override;

private:



};