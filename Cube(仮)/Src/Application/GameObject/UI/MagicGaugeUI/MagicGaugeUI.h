#pragma once

class MagicManager;

class MagicGaugeUI : public KdGameObject
{
public:
	MagicGaugeUI(){}
	~MagicGaugeUI()		override{}
	
	void Init()			override;
	void DrawSprite()	override;

private:

	static constexpr int m_texCount = 6;
	KdTexture m_texArray[m_texCount];

};