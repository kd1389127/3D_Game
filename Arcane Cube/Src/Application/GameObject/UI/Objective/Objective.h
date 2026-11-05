#pragma once

class Magicwand;
class Player;

// 画面左に「今やること」を1つずつ出すクラス(Map1用)。達成すると次の項目が出る
class Objective : public KdGameObject
{
public:
	Objective() {}
	~Objective() override {}

	void Init();
	void Update() override;
	void DrawSprite() override;

	void SetTarget(const std::shared_ptr<Magicwand>& wand) { m_wpWand = wand; }
	void SetPlayer(const std::shared_ptr<Player>& player) { m_wpPlayer = player; }

private:
	// 今の項目の段階
	enum class Phase
	{
		FadeIn,		// 現れる
		Wait,		// 達成を待つ
		Done,		// チェックを見せる
		FadeOut,	// 消える
		Finished,	// 全部終わった(何も出さない)
	};

	static constexpr int   kFontNo = 1;				// AddFontで登録した番号と合わせる
	static constexpr int   kAntiAlias = 3;			// 8倍アンチエイリアス(一番きれい)

	static constexpr int   kFadeFrames = 25;		// 現れる・消えるのにかける長さ
	static constexpr int   kDoneHoldFrames = 25;	// 達成後にチェックを見せる長さ

	static constexpr float kBoxHalf  = 12.0f;		// チェックボックスの大きさの半分
	static constexpr float kTextGap  = 12.0f;		// ボックスと文字の間
	static constexpr float kMargin   = 24.0f;		// 画面の左端からの余白
	static constexpr float kPosY     = 120.0f;		// 表示する高さ
	static constexpr float kBoxPosX  = -640.0f + kMargin + kBoxHalf;
	static constexpr float kTextLeft = kBoxPosX + kBoxHalf + kTextGap;	// 文字の左端

	static constexpr int kCount = 4;				// 狙う / ホイール / 撃つ / 光の柱へ向かう
	static constexpr const char* kTexts[kCount] =
	{
		"右クリック長押しで狙う",
		"マウスホイールで長さを決める",
		"左クリックで撃ってブロックを作る",
		"ゴールへ向かおう",
	};
	static constexpr int kGoalIndex = kCount - 1;	// 最後の項目(ゴールへ向かおう)

	static constexpr float kCheckSize          = 32.0f;	// チェックの大きさ(枠の24より少し大きく、枠からはみ出す)
	static constexpr int   kCheckPopFrames     = 8;		// チェックが大きくなる長さ
	static constexpr const char* kCheckTexPath = "Asset/Textures/UI/Objective/Check.png";
	KdTexture m_checkTex;

	// 達成の記録(項目を出す前にやっても残る)
	bool m_achieved[kCount] = {};
	std::weak_ptr<Magicwand> m_wpWand;
	std::weak_ptr<Player>	 m_wpPlayer;

	Phase m_phase = Phase::FadeIn;
	int   m_index = 0;	// 今出している項目の番号
	int   m_timer = 0;	// 今の段階に入ってからのフレーム数
	std::shared_ptr<KdFontSprite> m_text[kCount];
};