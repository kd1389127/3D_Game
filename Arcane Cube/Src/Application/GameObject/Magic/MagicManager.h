#pragma once
#include <algorithm>

// プレイヤーの「魔力」を管理するクラス
// 魔力の単位＝「杖を1回発動できる権利」(残り使用可能回数)
// 一度の発動で何段ブロックが生成されるか(1～5段)とは無関係に、消費は常に1固定
// (自動回復はせず、ブロックを手動で消した時にだけ回復する想定)
class MagicManager
{
public:
	static constexpr int MaxCastCount = 5;

	static MagicManager& Instance()
	{
		static MagicManager inst;
		return inst;
	}

	// 残り使用可能回数
	int GetRemainingCasts() const { return m_remainingCasts; }
	int GetMaxCasts() const { return MaxCastCount; }

	// 今、杖を１回発動できるか(消費しない、判定だけを取ってる)
	bool CanCast(int count = 1) const
	{
		return m_remainingCasts >= count;
	}

	// １回分の発動を消費する(固定１)。残りが無ければ何もせずfalseを返す
	bool TryConsumeCast(int count = 1)
	{
		if (!CanCast(count))
		{
			MarkAttemptFailed();
			return false;
		}
		m_remainingCasts -= count;
		return true;
	}

	// amountぶん回復する(上限MaxCastCountまで)。ブロックを手動で消した時に呼ぶ
	void Restore(int amount)
	{
		m_remainingCasts = std::min(MaxCastCount, m_remainingCasts + amount);
	}

	// 魔力が無いのに発動しようとした時に呼ぶ
	// (TryConsumeCastを呼ばずに事前ガードした場面用)
	// 呼ばれるたびにIDが進む。複数のUI(Reticle/MagicGaugeUI)が
	// それぞれ独立に「前回見たIDと違うか」を見て検知できるようにするため、
	// 一度読んだら消える単一フラグではなくカウンタにしている
	void MarkAttemptFailed()
	{
		m_attemptFailedId++;
	}

	// UI側が「前回チェックした時から失敗が起きたか」を判定するためのID
	// (読んでも消費されない。値の変化だけをUI側で見る)
	int GetAttemptFailedId() const { return m_attemptFailedId; }

	// ステージ開始時などに満タンへ戻す
	void Reset()
	{
		m_remainingCasts = MaxCastCount;
	}

private:
	MagicManager() = default;
	int  m_remainingCasts = MaxCastCount;
	int m_attemptFailedId = 0;

};