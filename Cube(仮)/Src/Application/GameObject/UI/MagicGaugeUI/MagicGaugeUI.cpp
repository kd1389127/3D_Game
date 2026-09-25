#include "MagicGaugeUI.h"
#include "../../Magic/MagicManager.h"

void MagicGaugeUI::Init()
{
	m_texArray[0].Load("Asset/Textures/UI/MagicGauge/gauge_0.png");
	m_texArray[1].Load("Asset/Textures/UI/MagicGauge/gauge_1.png");
	m_texArray[2].Load("Asset/Textures/UI/MagicGauge/gauge_2.png");
	m_texArray[3].Load("Asset/Textures/UI/MagicGauge/gauge_3.png");
	m_texArray[4].Load("Asset/Textures/UI/MagicGauge/gauge_4.png");
	m_texArray[5].Load("Asset/Textures/UI/MagicGauge/gauge_5.png");
}

void MagicGaugeUI::DrawSprite()
{
	int remaining = MagicManager::Instance().GetRemainingCasts();
	remaining = std::clamp(remaining, 0, m_texCount - 1);

	KdShaderManager::Instance().m_spriteShader.DrawTex(&m_texArray[remaining], -400, -280, 400, 80);

}