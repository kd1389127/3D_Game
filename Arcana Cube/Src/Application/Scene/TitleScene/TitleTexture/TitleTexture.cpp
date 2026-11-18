#include "TitleTexture.h"

void TitleTexture::Init()
{
	m_bgTex.Load("Asset/Textures/Title/Title.png");
	m_logoTex.Load("Asset/Textures/Title/TitleLogo.png");
	m_startTex.Load("Asset/Textures/Title/ClickToStart.png");
}

void TitleTexture::Update()
{
	m_frame++;
}

void TitleTexture::DrawSprite()
{
	auto& sp = KdShaderManager::Instance().m_spriteShader;

	// 背景
	sp.DrawTex(&m_bgTex, 0, 0, 1280, 720);

	// ロゴ
	const float floatY = std::sin(m_frame * 0.03f) * 4.0f;
	sp.DrawTex(&m_logoTex, 0, static_cast<int>(180 + floatY), 900, 300);

	// Click To Start
	const float alpha = 0.575f + 0.425f * std::sin(m_frame * 0.05f);
	Math::Color color = { 1,1,1,alpha };
	sp.DrawTex(&m_startTex, 0, -200, 420, 120, nullptr, &color);
}
