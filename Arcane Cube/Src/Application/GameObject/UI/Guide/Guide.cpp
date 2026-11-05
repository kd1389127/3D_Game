#include "Guide.h"
#include "../Mouse/Mouse.h"
#include "../../../Scene/SceneManager.h"

static const char* kTabTexPath = "Asset/Textures/UI/Guide/TabHint.png";

void Guide::Init()
{
	m_tabTex.Load(kTabTexPath);
	m_drawType = eDrawTypeUI;

	// Slide1.png〜Slide3.png をまとめて読む
	for (int i = 0; i < kSlideCount; ++i)
	{
		m_slideTex[i].Load("Asset/Textures/UI/Guide/Slide" + std::to_string(i + 1) + ".png");
	}
}

void Guide::OpenSlides()
{
	m_slideIndex = 0;
	Mouse::Instance().ShowMouse(true);	// クリックで進めるのでポインタを出す
}

void Guide::Update()
{
	if (m_openWait > 0)
	{
		--m_openWait;
		if (m_openWait == 0) { OpenSlides(); }
		return;
	}

	if (m_slideIndex < 0) { return; }	// スライドを出していない時は何もしない

	if (SceneManager::Instance().IsFading()) { return; }	// 暗転中は進めない
	if (!Mouse::Instance().IsClick()) { return; }

	++m_slideIndex;
	if (m_slideIndex >= kSlideCount)	// 最後の1枚で押したら閉じる
	{
		m_slideIndex = -1;
		Mouse::Instance().ShowMouse(false);
	}
}

void Guide::DrawSprite()
{
	auto& sp = KdShaderManager::Instance().m_spriteShader;

	// Tabアイコン(常時)
	Math::Color tabColor = { 1.0f, 1.0f, 1.0f, kTabAlpha };
	sp.DrawTex(&m_tabTex, kTabPosX, kTabPosY, kTabW, kTabH, nullptr, &tabColor);

	// 操作説明スライド(一番手前。周りは暗くしたゲーム画面のまま)
	if (m_slideIndex < 0) { return; }

	Math::Color dim = { 0.0f, 0.0f, 0.0f, kDimAlpha };
	sp.DrawBox(0, 0, 800, 450, &dim, true);
	sp.DrawTex(&m_slideTex[m_slideIndex], 0, 0, kSlideW, kSlideH);
}