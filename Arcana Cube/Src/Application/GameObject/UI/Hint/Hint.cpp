#include "Hint.h"
#include "../Guide/Guide.h"
#include "../../../Scene/SceneManager.h"
#include "../../../Scene/StageData.h"

namespace
{
	// アイコン画像
	const std::string kIconPaths[] =
	{
		"Asset/Textures/UI/Hint/HintIcon_Generate.png",	// Generate
		"Asset/Textures/UI/Hint/HintIcon_Carry.png",	// Carry
		"Asset/Textures/UI/Hint/HintIcon_Wall.png",		// Wall
		"Asset/Textures/UI/Hint/HintIcon_Lift.png",		// Lift
	};

	// 数がHintKindと食い違っていたらビルドで止める
	static_assert(std::size(kIconPaths) == static_cast<size_t>(HintKind::Count),
		"アイコンの数がHintKindと違います");
}

void Hint::Init(HintKind _hint, const std::shared_ptr<Guide>& _guide)
{
	m_drawType  = eDrawTypeUI;
	m_wpGuide   = _guide;
	m_hintIndex = static_cast<int>(_hint);

	// ヒントのないステージは、画像も読まず、何も出さない
	if (m_hintIndex == kNoHint) { return; }

	m_tex.Load(kIconPaths[m_hintIndex]);
	m_remainFrames = kShowFrames;
}

void Hint::Update()
{
	if (m_hintIndex == kNoHint) { return; }

	const bool nowH		 = (GetAsyncKeyState('H') & 0x8000) != 0;
	const bool isPressed = nowH && !m_prevH;
	m_prevH = nowH;

	// 暗転中は、時間を減らさず、Hも受け付けない
	if (SceneManager::Instance().IsFading()) { return; }

	if (isPressed)
	{
		const std::shared_ptr<Guide> spGuide = m_wpGuide.lock();
		if (spGuide)
		{
			spGuide->OpenSlides(Guide::kTopicCount + m_hintIndex);
		}

		m_remainFrames = 0;		// 見たら消す
		return;
	}

	if (m_remainFrames > 0) { --m_remainFrames; }
}

void Hint::DrawSprite()
{
	if (m_hintIndex == kNoHint) { return; }
	if (m_remainFrames <= 0) { return; }

	// 出し始めから何フレーム経ったか
	const int elapsedFrames = kShowFrames - m_remainFrames;

	// 出てくる前と消えていく側の透明度を求めて小さい方を使う
	const float fadeInAlpha  = std::min(1.0f, static_cast<float>(elapsedFrames) / kFadeInFrames);
	const float fadeOutAlpha = std::min(1.0f, static_cast<float>(m_remainFrames) / kFadeOutFrames);
	const float alpha	     = std::min(fadeInAlpha, fadeOutAlpha);

	const Math::Color color = { 1.0f,1.0f,1.0f,alpha };

	const float width  = static_cast<float>(m_tex.GetWidth()) * kIconScale;
	const float height = static_cast<float>(m_tex.GetHeight()) * kIconScale;

	auto& sp = KdShaderManager::Instance().m_spriteShader;
	sp.DrawTex(&m_tex, 0.0f, kIconY, width, height, nullptr, &color);
}
