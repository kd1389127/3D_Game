#pragma once

// 操作説明のUIを出すクラス。Tabアイコンは常時、スライドはMap1の開始時とTabメニューから出す
// スライドは「項目」ごとに分かれていて、項目ごとに枚数が違う
class Guide : public KdGameObject
{
public:
	// 項目の数(PauseMenu::kTopicCount と同じにする)
	static constexpr int kTopicCount = 3;

	Guide() {}
	~Guide()	override {}

	void Init();
	void Update()		override;
	void DrawSprite()	override;

	// 指定した項目のスライドを出す
	// _topic: 0=生成・削除 1=運ぶ・置く 2=ギミック(Map1の開始時は指定なしで0が出る)
	void OpenSlides(int _topic = 0);

	// スライドを見せている間はtrue(ゲーム側を止めるのに使う)
	bool IsSlideOpen() const { return m_slideIndex >= 0; }

private:
	// 今の項目のスライドが何枚あるか
	int GetSlideCount() const { return m_slideCount[m_topic]; }

	// 1つの項目に入れられるスライドの最大枚数(画像を増やす時は、これも増やす)
	static constexpr int kMaxSlides = 5;

	// 操作説明スライド(画面中央。画像1880x1160の縦横比のまま)
	static constexpr float kSlideW   = 800.0f;
	static constexpr float kSlideH   = kSlideW * 1160.0f / 1880.0f;	// 約494
	static constexpr float kDimAlpha = 0.6f;						// 周りのゲーム画面の暗さ

	static constexpr int kOpenDelay = 5;			// 開始から何フレーム待ってスライドを出すか
	int m_openWait = 0;								// 待ちの残りフレーム

	KdTexture m_tabTex;
	KdTexture m_slideTex[kTopicCount][kMaxSlides];	// 項目ごとのスライド画像
	int m_slideCount[kTopicCount] = {};				// 項目ごとの実際の枚数
	int m_topic = 0;								// 今出している項目
	int m_slideIndex = -1;							// -1=出していない / 0〜=その項目の何枚目を出しているか
};