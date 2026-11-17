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
	void SetOpen(bool _isOpen);

	// ページを切り替えて、並べるボタンを決める
	void SetPage(Page _page);

	// 操作説明の項目の数(Guide::kTopicCount と同じにする。Initで食い違いを検査している)
	static constexpr int kTopicCount = 3;

	// ステージ選択の並び(1列に5つ、最大2列 = 最大10ステージ)
	// 実際のステージの数は、cppで g_stageTable から求める
	static constexpr int kStageRows		  = 5;
	static constexpr int kMaxStageColumns = 2;
	static constexpr int kMaxStageCount   = kStageRows * kMaxStageColumns;

	// 画像の番号。m_tex[] の添字に使う
	// kTexTopic1 以降は項目1, 2, 3... の順、kTexStage1 以降は STAGE 1, 2, 3... の順に並ぶ
	enum TexId
	{
		kTexResume,
		kTexHowTo,
		kTexRestart,
		kTexStage,
		kTexTitle,
		kTexBack,
		kTexTopic1,
		kTexStage1 = kTexTopic1 + kTopicCount,
	};

	static constexpr int kTexCount = kTexStage1 + kMaxStageCount;

	// 一番多いのはステージ選択のページ(ステージ + 戻る)。他のページはこれより少ない
	static constexpr int kMaxBtn = kMaxStageCount + 1;

	static constexpr float kBtnW = 640.0f;			// ボタンの絵の大きさ(画像の幅1280の半分)
	static constexpr float kBtnH = 80.0f;
	static constexpr float kStep = 90.0f;			// ボタンの縦の間隔
	static constexpr float kOpenStep = 1.0f / 12.0f;	// 開閉の速さ(1フレームの進み。約0.2秒)

	// ステージ選択の、列と列の間隔(= ステージのボタンのクリックに反応する幅)
	// 画像の文字は幅の半分ほどなので、絵の幅(kBtnW)より狭くしても、文字は切れない
	static constexpr float kStageColumnStep = 420.0f;

	// ボタンの明るさ(1.0で変化なし)
	static constexpr float kBrightnessNormal  = 0.6f;	// 普通
	static constexpr float kBrightnessHover	  = 1.0f;	// マウスが乗っている
	static constexpr float kBrightnessLocked  = 0.25f;	// 未解放

	static constexpr float kSlideUpDistance   = 40.0f;	// 開く時に、下から上がってくる距離

	// ボタン1つ分の情報
	struct Button
	{
		int           texId	   = 0;						// 画像の番号(TexId)
		Math::Vector2 pos	   = Math::Vector2::Zero;	// 中心の位置(画面中央が原点、上がプラス)
		float         hitWidth = kBtnW;					// クリックに反応する幅(高さは kBtnH)
		bool          locked   = false;					// 未解放で選べないか
	};

	// ボタンを1つ足して、足したボタンを返す(位置や未解放の設定を、呼び出し側で続けて決めるため)
	Button& AddButton(int _texId, float _x, float _y, float _hitWidth = kBtnW);

	// ボタンを縦に並べた時の、_row番目(0が一番上)の中心Y。全体が画面の中央に来る
	static float GetRowY(int _row, int _rowCount);

	KdTexture m_tex[kTexCount];

	std::weak_ptr<Guide> m_wpGuide;

	// 今のページに並べるボタン
	Button m_buttons[kMaxBtn];
	int    m_count = 0;				// ボタンの数
	Page   m_page  = kPageMain;

	bool  m_isOpen  = false;
	float m_openT   = 0.0f;			// 開き具合(0=閉じている 1=開ききった)
	bool  m_prevTab = false;		// 前フレームのTabの状態
	int   m_hover   = -1;			// マウスが乗っているボタン(-1=なし)

	bool  m_returnToHowTo = false;	// 項目のスライドを見終わったら、項目選びのページに戻るか

};