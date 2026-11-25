#pragma once

class Guide;

// StageData.hで定義している。
enum class HintKind : int;

class Hint : public KdGameObject
{
public:
	Hint(){}
	~Hint()				override{}

	void Init(HintKind _hint, const std::shared_ptr<Guide>& _guide);
	void Update()		override;
	void DrawSprite()	override;

private:
	
	// アイコンの位置と大きさ
	static constexpr float kIconY	  = -170.0f;
	static constexpr float kIconScale = 1.0f;

	// 表示する長さ
	static constexpr int kShowFrames    = 240;	// 全体の長さ
	static constexpr int kFadeInFrames  = 30;	// 出てくる長さ
	static constexpr int kFadeOutFrames = 60;	// 消えていく長さ

	static constexpr int kNoHint = -1;

	KdTexture m_tex;
	std::weak_ptr<Guide> m_wpGuide;

	int m_hintIndex		= kNoHint;				// HintKindの番号
	int m_remainFrames  = 0;					// アイコンを出す残りフレーム
	bool m_prevH		= false;				// 前フレームのHの状態

};