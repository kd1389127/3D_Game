#include "Objective.h"
#include "../../../Scene/StageData.h"
#include "../../Weapon/Magicwand/Magicwand.h"
#include "../../Character/Player/Player.h"
#include "../../Map/Gimmick/Switch/Switch.h"
#include "../../Map/Gimmick/WandPickup/WandPickup.h"
#include "../Mouse/Mouse.h"

namespace
{
	// 種類を、配列の番号に変える(m_achievedやkTextsを引く時に使う)
	int ToIndex(ObjectiveKind kind)
	{
		return static_cast<int>(kind);
	}
}

void Objective::Init(const std::vector<ObjectiveKind>& items)
{
	m_drawType = eDrawTypeUI;

	m_items = items;
	if (m_items.empty())
	{
		m_phase = Phase::Finished;
	}

	for (int i = 0; i < kKindCount; ++i)
	{
		m_text[i] = KdFontManager::Instance().CreateFontTexture(kFontNo, kTexts[i], kAntiAlias);
	}
	m_checkTex.Load(kCheckTexPath);
	m_hintText = KdFontManager::Instance().CreateFontTexture(kFontNo, kHintText, kAntiAlias);
}

void Objective::Update()
{
	// 達成は、どの項目を出している時でも毎フレーム記録しておく
	// (項目が出る前にやってしまっても、出た時にチェックが付く)
	const std::shared_ptr<Magicwand> spWand = m_wpWand.lock();
	if (spWand)
	{
		if (spWand->IsAiming())			  { m_achieved[ToIndex(ObjectiveKind::Aim)] = true; }	
		if (spWand->GetStackCount() >= 2) { m_achieved[ToIndex(ObjectiveKind::Wheel)] = true; }
		if (spWand->HasGenerated())		  { m_achieved[ToIndex(ObjectiveKind::Shoot)] = true; }
	}

	// マウスを横に動かした量を足していく(一定を超えたら「見回した」)
	m_lookAmount += std::fabs(Mouse::Instance().GetDelta().x);
	if (m_lookAmount >= kLookGoal) { m_achieved[ToIndex(ObjectiveKind::Look)] = true; }

	// 台座の杖を取ったら達成
	const std::shared_ptr<WandPickup> spPickup = m_wpPickup.lock();
	if (spPickup && spPickup->IsPickedUp())
	{
		m_achieved[ToIndex(ObjectiveKind::GetWand)] = true;
	}

	const std::shared_ptr<Player> spPlayer = m_wpPlayer.lock();
	if (spPlayer)
	{
		// ゴールに触れたら、最後の項目を達成にする
		if (spPlayer->HasReachedGoal())
		{
			m_achieved[ToIndex(ObjectiveKind::Goal)] = true;
		}

		// ブロックを持ったら達成(Eキーで掴んだ瞬間にチェックが付く)
		if (spPlayer->IsCarryingBlock())
		{
			m_achieved[ToIndex(ObjectiveKind::Grab)] = true;
		}

		if (spPlayer->HasJumped()) { m_achieved[ToIndex(ObjectiveKind::Jump)] = true; }
	}

	// プレイヤーが動いたか(上下は無視。着地の補正で揺れても「動いた」にしないため)
	bool isMoving = false;
	if (spPlayer)
	{
		const Math::Vector3 pos = spPlayer->GetPos();
		if (m_hasPrevPos)
		{
			Math::Vector3 delta = pos - m_prevPlayerPos;
			delta.y = 0.0f;
			const float moved = delta.Length();	// 2回使うので変数にする
			isMoving = moved > kMoveEpsilon;
			m_walkDistance += moved;
		}

		m_prevPlayerPos = pos;
		m_hasPrevPos	= true;
	}

	if (m_walkDistance >= kWalkGoal) { m_achieved[ToIndex(ObjectiveKind::Move)] = true; }

	// 動いたらカウントをリセット、止まっている間は数える
	if (isMoving) { m_idleFrames = 0; }
	else { ++m_idleFrames; }

	// 止まって一定時間たったら出す。出したあとは、動いても数秒は出したままにする
	// (動きながらだと出たり消えたりして落ち着かないため)
	const bool isIdle = (m_idleFrames >= kHintIdleFrames);
	if (isIdle) { m_holdFrames = kHoldFrames; }	// 止まっている間は満タンに保つ
	else if (m_holdFrames > 0) { --m_holdFrames; }				// 動き出したら減らす
	const bool isShown = isIdle || m_holdFrames > 0;

	// Tab：メニュー
	if (isShown) { m_hintT = std::min(m_hintT + kHintStep, 1.0f); }
	else { m_hintT = std::max(m_hintT - kHintStep, 0.0f); }

	// オブジェクティブは、達成のチェックを見せている間も出し続ける
	// (歩きながら達成しても、チェックが付く所を見逃さないため)
	const bool isShowingCheck = (m_phase == Phase::Done || m_phase == Phase::FadeOut);
	if (isShown || isShowingCheck) { m_objT = std::min(m_objT + kObjStep, 1.0f); }
	else { m_objT = std::max(m_objT - kObjStep, 0.0f); }

	// すべてのスイッチが同時に起動したら達成(スイッチがないステージでは何もしない)
	if (!m_wpSwitches.empty())
	{
		bool allActive = true;
		for (const auto& wpSwitch : m_wpSwitches)
		{
			const std::shared_ptr<Switch> spSwitch = wpSwitch.lock();
			if (!spSwitch || !spSwitch->IsActive())
			{
				allActive = false;
			}
		}

		if (allActive)
		{
			// 「スイッチを起動する」系は、どれも同じ条件(全スイッチが起動)で達成にする
			// (ステージ2・3はスイッチが1つなので、「全部」=「1つ」)
			m_achieved[ToIndex(ObjectiveKind::OneSwitch)]  = true;
			m_achieved[ToIndex(ObjectiveKind::LiftSwitch)] = true;
			m_achieved[ToIndex(ObjectiveKind::Switches)]   = true;
		}
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
		// 今の項目が記録済みなら、チェックの段階へ
		if (m_achieved[ToIndex(m_items[m_index])])
		{
			m_timer = 0;
			m_phase = Phase::Done;
		}
		break;

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
			++m_index;
			m_timer = 0;

			// 最後まで終わったら止める(m_text[m_index]で配列の外を読まないため)
			if (m_index >= static_cast<int>(m_items.size()))
			{
				m_phase = Phase::Finished;
			}
			else
			{
				m_phase = Phase::FadeIn;
				m_holdFrames = kHoldFrames;	// 次の項目は、動いていても数秒は出したままにする
			}
		}
		break;

	case Phase::Finished:
		break;
	}
}

void Objective::DrawSprite()
{
	// Tab：メニュー(目標が全部終わっても出す)
	if (m_hintT > 0.0f)
	{
		const float alpha = KdEase::OutCubic(m_hintT);
		const float slideX = -kSlideDist * (1.0f - KdEase::OutBack(m_hintT));	// 左から入る
		DrawFontText(m_hintText, kHintLeft + slideX, kHintPosY, { 1.0f, 1.0f, 1.0f, kHintAlpha * alpha });
	}

	if (m_phase == Phase::Finished) { return; }

	auto& sp = KdShaderManager::Instance().m_spriteShader;

	// 項目の入れ替わりによる出入り(0=見えない 1=見える)
	float phaseT = 1.0f;
	if (m_phase == Phase::FadeIn) { phaseT = static_cast<float>(m_timer) / kFadeFrames; }
	else if (m_phase == Phase::FadeOut) { phaseT = 1.0f - static_cast<float>(m_timer) / kFadeFrames; }

	// 動きによる出入りと掛け合わせて、1つの進み具合にする
	const float appear = phaseT * m_objT;
	const float alpha = KdEase::OutCubic(appear);	// 透明→不透明
	const float slideX = -kSlideDist * (1.0f - KdEase::OutBack(appear));	// 左から入る

	// チェックボックスの枠
	Math::Color frame = { 1.0f, 1.0f, 1.0f, 0.9f * alpha };
	sp.DrawBox(kBoxPosX + slideX, kPosY, kBoxHalf, kBoxHalf, &frame, false);

	// 達成後(Done・FadeOut)は、チェックを描く
	if (m_phase == Phase::Done || m_phase == Phase::FadeOut)
	{
		// Doneの最初だけ、小さい所から大きくなる
		float scale = 1.0f;
		if (m_phase == Phase::Done && m_timer < kCheckPopFrames)
		{
			const float t = static_cast<float>(m_timer) / kCheckPopFrames;
			scale = KdEase::OutCubic(t);
		}

		const float size = kCheckSize * scale;
		Math::Color checkColor = { 1.0f, 1.0f, 1.0f, alpha };
		sp.DrawTex(&m_checkTex, kBoxPosX + slideX, kPosY, size, size, nullptr, &checkColor);
	}

	Math::Color textColor = { 1.0f, 1.0f, 1.0f, alpha };
	DrawFontText(m_text[ToIndex(m_items[m_index])], kTextLeft + slideX, kPosY, textColor);
}

void Objective::DrawFontText(const std::shared_ptr<KdFontSprite>& font, float left, float y, const Math::Color& color)
{
	auto& sp = KdShaderManager::Instance().m_spriteShader;

	// 全文字を、ずらした位置で描く(影→文字の順に2回呼ぶ)
	auto drawAll = [&](float offsetX, float offsetY, const Math::Color& drawColor)
	{
		float x = left + offsetX;
		for (const auto& glyph : font->GetTexList())
		{
			const float w = static_cast<float>(glyph->FontTex->GetInfo().Width);
			const float h = static_cast<float>(glyph->FontTex->GetInfo().Height);
			sp.DrawTex(glyph->FontTex.get(), x + w / 2, y + offsetY, w, h, nullptr, &drawColor);	// 座標は中心指定
			x += w;
		}
	};

	const Math::Color shadowColor = { 0.0f, 0.0f, 0.0f, color.A() * kShadowAlpha };
	drawAll(kShadowOffset, -kShadowOffset, shadowColor);	// 右下にずらした影
	drawAll(0.0f, 0.0f, color);
}

void Objective::SetSwitches(const std::vector<std::shared_ptr<Switch>>& switches)
{
	for (const auto& sw : switches)
	{
		m_wpSwitches.push_back(sw);
	}
}
