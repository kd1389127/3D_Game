#include "Objective.h"
#include "../../Weapon/Magicwand/Magicwand.h"

void Objective::Init()
{
	m_drawType = eDrawTypeUI;

	// 文字のテクスチャは1回だけ作って持っておく(毎フレーム作ると重い)
	for (int i = 0; i < kCount; ++i)
	{
		m_text[i] = KdFontManager::Instance().CreateFontTexture(kFontNo, kTexts[i], kAntiAlias);
	}
	m_checkTex.Load(kCheckTexPath);
}

void Objective::Update()
{
	// 達成は、どの項目を出している時でも毎フレーム記録しておく
	// (項目が出る前にやってしまっても、出た時にチェックが付く)
	const std::shared_ptr<Magicwand> spWand = m_wpWand.lock();
	if (spWand)
	{
		if (spWand->IsAiming()) { m_achieved[0] = true; }			// 狙った
		if (spWand->GetStackCount() >= 2) { m_achieved[1] = true; }	// ホイールで長さを変えた
		if (spWand->HasGenerated()) { m_achieved[2] = true; }		// ブロックを作れた
	}

	switch (m_phase)
	{
	case Phase::FadeIn:
		++m_timer;
		if (m_timer >= kFadeFrames)
		{
			m_timer = 0;
			m_phase = Phase::Wait;
		}
		break;

	case Phase::Wait:
	{
		// 今の項目が記録済みなら、チェックの段階へ(4つ目は記録がないので、出たまま残る)
		if (m_achieved[m_index])
		{
			m_timer = 0;
			m_phase = Phase::Done;
		}
		break;
	}

	case Phase::Done:
		++m_timer;
		if (m_timer >= kDoneHoldFrames)
		{
			m_timer = 0;
			m_phase = Phase::FadeOut;
		}
		break;

	case Phase::FadeOut:
		++m_timer;
		if (m_timer >= kFadeFrames)
		{
			++m_index;	// 次の項目へ(達成のある項目だけここに来るので、範囲は超えない)
			m_timer = 0;
			m_phase = Phase::FadeIn;
		}
		break;
	}
}

void Objective::DrawSprite()
{
	auto& sp = KdShaderManager::Instance().m_spriteShader;

	// 現れる・消える時の透明度(イージング：はじめは速く、終わりはゆっくり)
	float alpha = 1.0f;
	if (m_phase == Phase::FadeIn || m_phase == Phase::FadeOut)
	{
		const float t = static_cast<float>(m_timer) / kFadeFrames;	// 0〜1
		const float inverse = 1.0f - t;
		const float eased = 1.0f - inverse * inverse * inverse;		// EaseOutCubic

		if (m_phase == Phase::FadeIn) { alpha = eased; }
		else { alpha = 1.0f - eased; }
	}

	// チェックボックスの枠
	Math::Color frame = { 1.0f, 1.0f, 1.0f, 0.9f * alpha };
	sp.DrawBox(kBoxPosX, kPosY, kBoxHalf, kBoxHalf, &frame, false);

	// 達成後(Done・FadeOut)は、チェックを描く
	if (m_phase == Phase::Done || m_phase == Phase::FadeOut)
	{
		// Doneの最初だけ、小さい所から大きくなる
		float scale = 1.0f;
		if (m_phase == Phase::Done && m_timer < kCheckPopFrames)
		{
			const float t = static_cast<float>(m_timer) / kCheckPopFrames;
			const float inverse = 1.0f - t;
			scale = 1.0f - inverse * inverse * inverse;	// EaseOutCubic
		}

		const float size = kCheckSize * scale;
		Math::Color checkColor = { 1.0f, 1.0f, 1.0f, alpha };
		sp.DrawTex(&m_checkTex, kBoxPosX, kPosY, size, size, nullptr, &checkColor);	// 座標は中心指定
	}

	// 文字を1文字ずつ横に並べて描く(1文字の幅は、そのテクスチャの幅)
	Math::Color textColor = { 1.0f, 1.0f, 1.0f, alpha };
	float x = kTextLeft;
	for (const auto& glyph : m_text[m_index]->GetTexList())
	{
		const float w = static_cast<float>(glyph->FontTex->GetInfo().Width);
		const float h = static_cast<float>(glyph->FontTex->GetInfo().Height);
		sp.DrawTex(glyph->FontTex.get(), x + w / 2, kPosY, w, h, nullptr, &textColor);	// 座標は中心指定
		x += w;
	}
}