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

	// ここだけ触れば調整できる(x=0は中央、yは上がプラス)
	constexpr float kLogoW = 800.0f;
	constexpr float kLogoH = kLogoW * 106.0f / 839.0f;
	constexpr float kLogoY = 150.0f;

	constexpr float kKeyW = 450.0f;
	constexpr float kKeyH = kKeyW * 85.0f / 439.0f;
	constexpr float kKeyY = -225.0f;

	// 背景(画面いっぱい)
	sp.DrawTex(&m_bgTex, 0, 0, 1280, 720);

	// ロゴ：最初の60フレームでふわっと現れる
	const float logoAlpha = std::min(m_frame / 60.0f, 1.0f);
	Math::Color logoColor = { 1, 1, 1, logoAlpha };
	sp.DrawTex(&m_logoTex, 0, kLogoY, kLogoW, kLogoH, nullptr, &logoColor);

	// Press Any Key：90フレームで現れてから点滅
	const float fadeIn = std::min(m_frame / 90.0f, 1.0f);
	const float blink = 0.7f + 0.3f * std::sin(m_frame * 0.05f);	// 0.4〜1.0で点滅
	Math::Color keyColor = { 1, 1, 1, fadeIn * blink };
	sp.DrawTex(&m_keyTex, 0, kKeyY, kKeyW, kKeyH, nullptr, &keyColor);
}