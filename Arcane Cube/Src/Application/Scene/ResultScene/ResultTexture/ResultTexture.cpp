#include "ResultTexture.h"

void ResultTexture::Init()
{
	m_bgTex.Load("Asset/Textures/Result/Result.png");
	m_logoTex.Load("Asset/Textures/Result/GameClearLogo.png");
	m_keyTex.Load("Asset/Textures/Result/PressAnyKey.png");
}

void ResultTexture::Update()
{
	m_frame++;
}

void ResultTexture::DrawSprite()
{
	auto& sp = KdShaderManager::Instance().m_spriteShader;

	// 背景(画面いっぱい)
	sp.DrawTex(&m_bgTex, 0, 0, 1280, 720);

	// ロゴ：最初の60フレームでふわっと現れる
	const float logoAlpha = std::min(m_frame / 60.0f, 1.0f);
	Math::Color logoColor = { 1, 1, 1, logoAlpha };
	sp.DrawTex(&m_logoTex, 0, 230, 839, 106, nullptr, &logoColor);

	// Press Any Key：入力待ちの90フレームで現れてから点滅
	const float fadeIn = std::min(m_frame / 90.0f, 1.0f);
	const float blink = 0.575f + 0.425f * std::sin(m_frame * 0.05f);
	Math::Color keyColor = { 1, 1, 1, fadeIn * blink };
	sp.DrawTex(&m_keyTex, 0, -279, 336, 65, nullptr, &keyColor);
}