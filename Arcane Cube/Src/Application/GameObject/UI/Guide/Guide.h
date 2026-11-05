#pragma once

// 操作説明のUIを出すクラス。Tabアイコンは常時、スライドはMap1の開始時とTabメニューから出す
class Guide : public KdGameObject
{
public:
	Guide() {}
	~Guide()	override {}

	void Init();
	void Update()		override;
	void DrawSprite()	override;

	// スライドを出す(Map1の開始時と、Tabメニューの「HOW TO PLAY」から呼ぶ)
	void OpenSlides();

	// スライドを見せている間はtrue(ゲーム側を止めるのに使う)
	bool IsSlideOpen() const { return m_slideIndex >= 0; }

private:
	// Tabアイコン(画面右上、常時表示)
	static constexpr float kTabW = 84.0f;
	static constexpr float kTabH = 56.0f;
	static constexpr float kTabAlpha = 0.75f;	// 少し透かして、目立ちすぎないようにする
	static constexpr float kTabPosX = 640.0f - 24.0f - kTabW / 2;
	static constexpr float kTabPosY = 360.0f - 20.0f - kTabH / 2;

	// 操作説明スライド(画面中央。画像1880x1160の縦横比のまま)
	static constexpr int   kSlideCount = 3;
	static constexpr float kSlideW = 800.0f;
	static constexpr float kSlideH = kSlideW * 1160.0f / 1880.0f;	// 約494
	static constexpr float kDimAlpha = 0.6f;						// 周りのゲーム画面の暗さ

	static constexpr int kOpenDelay = 5;	// 開始から何フレーム待ってスライドを出すか
	int m_openWait = 0;						// 待ちの残りフレーム

	KdTexture m_tabTex;
	KdTexture m_slideTex[kSlideCount];
	int m_slideIndex = -1;	// -1=出していない / 0〜=何枚目を出しているか
};