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
	bool CanCast() const
	{
		return m_remainingCasts > 0;
	}

	// １回分の発動を消費する(固定１)。残りが無ければ何もせずfalseを返す
	bool TryConsumeCast()
	{
		if (!CanCast())
		{
			m_lastAttemptFailed = true;	// UI側で「魔力が足りない」表示に使うフラグ
			return false;
		}
		m_remainingCasts -= 1;
		return true;
	}

	// amountぶん回復する(上限MaxCastCountまで)。ブロックを手動で消した時に呼ぶ
	void Restore(int amount)
	{
		m_remainingCasts = std::min(MaxCastCount, m_remainingCasts + amount);
	}

	// 魔力が無いのに発動しようとした時に呼ぶ
	// (TryConsumeCastを呼ばずに事前ガードした場面用)
	void MarkAttemptFailed()
	{
		m_lastAttemptFailed = true;
	}

	// UI側が「魔力がありません」メッセージを一度だけ表示するためのフラグ取得(読むとリセットされる)
	bool ConsumeAttemptFailedFlag()
	{
		bool f = m_lastAttemptFailed;
		m_lastAttemptFailed = false;
		return f;
	}

	// ステージ開始時などに満タンへ戻す
	void Reset()
	{
		m_remainingCasts = MaxCastCount;
		m_lastAttemptFailed = false;
	}

private:
	MagicManager() = default;
	int  m_remainingCasts = MaxCastCount;
	bool m_lastAttemptFailed = false;

};