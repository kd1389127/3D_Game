#pragma once

class PauseMenu : public KdGameObject
{
public:
	PauseMenu() {}
	~PauseMenu()	override {}

	void Init()			override;
	void Update()		override;
	void DrawSprite()	override;

	// メニューが開いているか(GameSceneがゲーム側を止めるのに使う)
	bool IsOpen() const { return m_isOpen; }

private:
	// 開閉の切り替え。Tabで開く・Tabで閉じる・「再開」の3か所から呼ぶので関数にした
	void SetOpen(bool _isOpen);

	// ページ(0=メインメニュー 1=ステージ選択)を切り替えて、並べるボタンを決める
	void SetPage(int _page);

	// 画像の番号。m_tex[] の添字に使う(kTexStage1 以降は STAGE 1, 2, 3... の順に並ぶ)
	enum TexId
	{
		kTexResume,
		kTexRestart,
		kTexStage,
		kTexTitle,
		kTexBack,
		kTexStage1,
	};

	// ステージの数。g_stageTable の数と合わせる(チュートリアルを足したら増やす)
	static constexpr int kStageCount = 5;
	static constexpr int kTexCount = kTexStage1 + kStageCount;
	static constexpr int kMaxBtn = kStageCount + 1;	// 一番多いページ(STAGE 1〜5 と BACK)

	static constexpr float kBtnW = 480.0f;	// ボタンの大きさ(画像の幅960の半分)
	static constexpr float kBtnH = 80.0f;
	static constexpr float kStep = 90.0f;	// ボタンの間隔
	static constexpr float kOpenStep = 1.0f / 12.0f;	// 開閉の速さ(1フレームの進み。約0.2秒)

	KdTexture m_tex[kTexCount];

	// 今のページに並べるボタン(上から順)
	int  m_ids[kMaxBtn] = {};		// 画像の番号
	bool m_locked[kMaxBtn] = {};	// 未解放で選べないか
	int  m_count = 0;				// ボタンの数
	int  m_page = 0;

	bool  m_isOpen = false;
	float m_openT = 0.0f;		// 開き具合(0=閉じている 1=開ききった)
	bool  m_prevTab = false;	// 前フレームのTabの状態
	int   m_hover = -1;			// マウスが乗っているボタン(-1=なし)
};