#pragma once

class Guide;

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

	// 操作説明スライドを出す相手
	void SetGuide(const std::shared_ptr<Guide>& guide) { m_wpGuide = guide; }

private:
	// ページの種類
	enum Page
	{
		kPageMain,		// メインメニュー
		kPageStage,		// ステージ選択
		kPageHowTo,		// 操作説明の項目選び
	};

	// 開閉の切り替え。Tabで開く・Tabで閉じる・「再開」の3か所から呼ぶので関数にした
	void SetOpen(bool _isOpen, bool _keepMouse = false);

	// ページを切り替えて、並べるボタンを決める
	void SetPage(Page _page);

	// 操作説明の項目の数(Guide::kTopicCount と同じにする。Initで食い違いを検査している)
	static constexpr int kTopicCount = 3;

	// ステージ選択で出せるステージの上限(写真・名前・ステージ番号)
	// 実際のステージの数は、cppでg_stageTableから求める
	static constexpr int kMaxStageCount = 10;

	// 画像の番号。m_tex[] の添字に使う
	// kTexTopic1 以降は 項目1, 2, 3... の順
	// kTexStage1・kTexPhoto1・kTexStageName1 以降は STAGE 1, 2, 3... の順に並ぶ
	enum TexId
	{
		kTexResume,
		kTexHowTo,
		kTexRestart,
		kTexStage,
		kTexTitle,
		kTexBack,
		kTexPauseTitle,
		kTexTitleLine,
		kTexCornerLine,
		kTexHoverBar,
		kTexArrowLeft,
		kTexArrowRight,
		kTexBackIcon,
		kTexLock,
		kTexDiamond,
		kTexPhotoFrame,
		kTexTopic1,
		kTexStage1 = kTexTopic1 + kTopicCount,
		kTexPhoto1 = kTexStage1 + kMaxStageCount,
		kTexStageName1 = kTexPhoto1 + kMaxStageCount,
	};

	static constexpr int kTexCount = kTexStageName1 + kMaxStageCount;

	// 一番多いのはメインのページ(5つ)。他のページはこれ以下
	static constexpr int kMaxBtn = 5;

	// ボタンの配置
	static constexpr float kBtnW	 = 640.0f;			// ボタンの絵の大きさ(画像の幅1280の半分)
	static constexpr float kBtnH	 = 80.0f;
	static constexpr float kStep	 = 90.0f;			// ボタンの縦の間隔
	static constexpr float kOpenStep = 1.0f / 12.0f;	// 開閉の速さ(1フレームの進み。約0.2秒)

	// ステージ選択の配置
	static constexpr float kPhotoY			= 90.0f;	// 写真と矢印の中心Y
	static constexpr float kPhotoW			= 696.0f;	// 写真の大きさ(16:9。枠の内側に収まる大きさ)
	static constexpr float kPhotoH			= 391.5f;
	static constexpr float kFrameW			= 720.0f;	// 写真を囲む金の枠
	static constexpr float kFrameH			= 405.0f;
	static constexpr float kArrowX			= 450.0f;	// 矢印の中心X(左右対称)
	static constexpr float kArrowSize		= 80.0f;
	static constexpr float kNameY			= -165.0f;	// ステージ名の中心Y
	static constexpr float kLabelY			= -222.0f;	// 「ステージ n」の中心Y(点の少し上)
	static constexpr float kDotsY			= -270.0f;	// 下の点の中心Y
	static constexpr float kDotStep			= 30.0f;	// 点と点の間隔
	static constexpr float kDotSize			= 14.0f;
	static constexpr float kDotSelectedSize = 28.0f;	// 今のステージの点
	static constexpr float kLockSize	    = 140.0f;

	// メインのページの「ポーズ」と飾り線
	static constexpr float kPauseTitleY     = 230.0f;	// 「ポーズ」の中心Y
	static constexpr float kPauseTitleLineY = 192.0f;	// 「ポーズ」の下の線の中心Y
	static constexpr float kPauseTitleLineW = 320.0f;
	static constexpr float kPauseTitleLineH = 24.0f;

	// マウスが乗ったボタンの、青い帯と◆(帯の幅は全ボタン共通。一番長い「ステージ選択」に合わせた)
	static constexpr float kBarHalfW		  = 300.0f;	// 帯の半分の幅
	static constexpr int   kBarHalfH		  = 40;		// 帯の半分の高さ(HoverBar.png の高さ80の半分)
	static constexpr int   kBarCapW			  = 40;		// 帯の左右の角の幅(HoverBar.png の角の部分)
	static constexpr int   kBarTexW			  = 640;	// HoverBar.png の幅
	static constexpr float kHoverDiamondInset = 0.0f;	// ◆を帯のふちから内側へ寄せる距離
	static constexpr float kHoverDiamondSize  = 24.0f;

	// 画面の四隅に置く小さめのボタン(戻る・操作説明・タイトルへ)
	static constexpr float kCornerX			  = 520.0f;	// 中心X(左右対称)
	static constexpr float kCornerY		      = -317.0f;// 中心Y(下)
	static constexpr float kCornerW			  = 400.0f;
	static constexpr float kCornerH			  = 50.0f;
	static constexpr float kCornerLineOffsetY = -30.0f;	// ボタンの中心から、下線までのずれ
	static constexpr float kCornerLineW		  = 240.0f;
	static constexpr float kCornerLineH       = 8.0f;
	static constexpr float kBackIconOffsetX   = -75.0f;	// 戻るアイコンの、ボタン中心からのずれ
	static constexpr float kBackIconSize      = 46.0f;

	// ボタンの明るさ(1.0で変化なし)
	static constexpr float kBrightnessNormal = 0.6f;	// 普通
	static constexpr float kBrightnessHover  = 1.0f;	// マウスが乗っている
	static constexpr float kBrightnessLocked = 0.25f;	// 未解放

	static constexpr float kSlideUpDistance  = 40.0f;	// 開く時に、下から上がってくる距離

	// ボタン1つ分の情報
	struct Button
	{
		int           texId    = 0;						// 画像の番号(TexId)
		Math::Vector2 pos      = Math::Vector2::Zero;	// 中心の位置(画面中央が原点、上がプラス)
		float         width    = kBtnW;					// 絵の大きさ(クリックに反応する大きさでもある)
		float         height   = kBtnH;
		bool          locked   = false;					// 未解放で選べないか
		bool          hasFrame = false;					// マウスが乗った時に青い帯を出すか
	};

	// ボタンを1つ足して、足したボタンを返す(位置や未解放の設定を、呼び出し側で続けて決めるため)
	Button& AddButton(int _texId, float _x, float _y, float _width = kBtnW, float _height = kBtnH);

	// ボタンを縦に並べた時の、_row番目(0が一番上)の中心Y。全体が画面の中央に来る
	static float GetRowY(int _row, int _rowCount);

	// 青い帯を描く。左右の角は元の大きさのまま、真ん中だけ伸ばす
	void DrawHoverBar(float _centerX, float _y, float _halfW, const Math::Color& _color);

	KdTexture m_tex[kTexCount];

	std::weak_ptr<Guide> m_wpGuide;

	// 今のページに並べるボタン
	Button m_buttons[kMaxBtn];
	int    m_count = 0;				// ボタンの数
	Page   m_page = kPageMain;

	bool  m_isOpen  = false;
	float m_openT   = 0.0f;			// 開き具合(0=閉じている 1=開ききった)
	bool  m_prevTab = false;		// 前フレームのTabの状態
	int   m_hover   = -1;			// マウスが乗っているボタン(-1=なし)

	bool  m_returnToHowTo = false;	// 項目のスライドを見終わったら、項目選びのページに戻るか

	int   m_selectStage = 0;		// ステージ選択で見ているステージ(0始まり)
};