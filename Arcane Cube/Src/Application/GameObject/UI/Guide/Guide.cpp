#include "Guide.h"

static const char* kTexPath = "Asset/Textures/UI/Guide/Operation.png";
static const char* kTabTexPath = "Asset/Textures/UI/Guide/TabHint.png";

void Guide::Init(bool showBanner)
{
	m_tex.Load(kTexPath);
	m_tabTex.Load(kTabTexPath);
	m_drawType = eDrawTypeUI;
	m_showBanner = showBanner;
	m_showTimer = showBanner ? kShowFrames : 0;	// バナー無しなら、最初から出さない
}
void Guide::Update()
{
	if (!m_showBanner) { return; }

	const std::shared_ptr<KdGameObject> spTarget = m_wpTarget.lock();
	if (!spTarget) { return; }

	// 前のフレームから動いたか(視点を回すだけなら「止まっている」扱い)
	const Math::Vector3 pos = spTarget->GetPos();
	const bool moved = (pos - m_prevPos).LengthSquared() > kMoveEps;
	m_prevPos = pos;

	if (m_showTimer > 0)
	{
		--m_showTimer;		// 出している間は、時間を減らすだけ
	}
	else
	{
		// 消えている間だけ、止まっている時間を数える(動いたらやり直し)
		m_idleTimer = moved ? 0 : m_idleTimer + 1;
		if (m_idleTimer >= kIdleFrames)
		{
			m_showTimer = kShowFrames;	// 止まりすぎたので、もう一度見せる
			m_idleTimer = 0;
		}
	}

	// 出ている間は1へ、消えている間は0へ、少しずつ近づける(フェード)
	const float target = (m_showTimer > 0) ? 1.0f : 0.0f;
	if (m_alpha < target) { m_alpha = std::min(m_alpha + kFadeStep, target); }
	else if (m_alpha > target) { m_alpha = std::max(m_alpha - kFadeStep, target); }
}

void Guide::DrawSprite()
{
	auto& sp = KdShaderManager::Instance().m_spriteShader;

	// Tabアイコン(常時)
	Math::Color tabColor = { 1.0f, 1.0f, 1.0f, kTabAlpha };
	sp.DrawTex(&m_tabTex, kTabPosX, kTabPosY, kTabW, kTabH, nullptr, &tabColor);

	// 操作説明のバナー(Map1だけ、出ている間)
	if (m_alpha <= 0.0f) { return; }
	Math::Color color = { 1.0f, 1.0f, 1.0f, m_alpha };
	sp.DrawTex(&m_tex, kPosX, kPosY, kW, kH, nullptr, &color);
}