#pragma once

class Magicwand;
class Player;
class Switch;
class WandPickup;
enum class ObjectiveKind : int;

// 画面左に「今やること」を1つずつ出すクラス(Map1用)。達成すると次の項目が出る
class Objective : public KdGameObject
{
public:
	Objective() {}
	~Objective() override {}

	void Init(const std::vector<ObjectiveKind>& items);
	void Update() override;
	void DrawSprite() override;
	
	// 文字を1文字ずつ横に並べて描く(left=左端のX、y=中心のY)
	void DrawFontText(const std::shared_ptr<KdFontSprite>& font, float left, float y, const Math::Color& color);

	void SetTarget(const std::shared_ptr<Magicwand>& wand) { m_wpWand = wand; }
	void SetPlayer(const std::shared_ptr<Player>& player) { m_wpPlayer = player; }
	void SetSwitches(const std::vector<std::shared_ptr<Switch>>& switches);
	void SetWandPickup(const std::shared_ptr<WandPickup>& pickup) { m_wpPickup = pickup; }

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
	static constexpr float kPosY     = 50.0f;		// 表示する高さ
	static constexpr float kBoxPosX  = -640.0f + kMargin + kBoxHalf;
	static constexpr float kTextLeft = kBoxPosX + kBoxHalf + kTextGap;	// 文字の左端

	// ObjectiveKindの種類の数。文言の並びは、StageData.hのenumと同じにする
	static constexpr int kKindCount = 12;
	static constexpr const char* kTexts[kKindCount] =
	{
		"右クリック長押しで狙う",
		"マウスホイールで長さを決める",
		"左クリックで撃ってブロックを作る",
		"ゴールへ向かおう",
		"Eキーでブロックを掴もう",
		"スイッチを起動して檻を開けよう",
		"リフトに乗りスイッチを起動しよう",
		"すべてのスイッチを起動して檻を開けよう",
		"マウスで見回そう",
		"WASDキーで移動しよう",
		"スペースキーでジャンプしよう",
		"杖を狙って、Eキーか左クリックで取ろう",
	};

	// チュートリアルの達成条件
	static constexpr float kLookGoal = 300.0f;	// マウスを横に動かした合計(約45度ぶん)
	static constexpr float kWalkGoal = 24.0f;	// 歩いた合計の距離(ブロック3個ぶん)
	float m_lookAmount				 = 0.0f;
	float m_walkDistance			 = 0.0f;

	// 「Tab：メニュー」(オブジェクティブの下。動いたら消え、止まったら出る)
	static constexpr const char* kHintText	= "Tab : メニュー";
	static constexpr float kHintGapY		= 40.0f;				// オブジェクティブからの下がり幅
	static constexpr float kHintPosY		= kPosY - kHintGapY;
	static constexpr float kHintLeft		= kBoxPosX - kBoxHalf;	// ボックスの左端にそろえる
	static constexpr float kHintAlpha		= 0.75f;				// 少し透かして目立ちすぎないように
	static constexpr float kHintStep		= 1.0f / 15.0f;			// 出る・消えるの速さ(約0.25秒)
	static constexpr float kShadowOffset	= 2.0f;					// 影をずらす距離
	static constexpr float kShadowAlpha		= 0.6f;					// 影の濃さ(文字の透明度に掛ける)
	static constexpr int   kHintIdleFrames  = 120;					// 止まってから出るまで(約1秒)
	static constexpr float kMoveEpsilon		= 0.05f;				// これ以下の動きは「止まっている」扱い
	static constexpr int   kHoldFrames		= 120;					// 出したあと、動いても出したままにする長さ(約3秒)

	// 目標を完了を表すチェック
	static constexpr float kCheckSize          = 32.0f;	// チェックの大きさ(枠の24より少し大きく、枠からはみ出す)
	static constexpr int   kCheckPopFrames     = 8;		// チェックが大きくなる長さ
	static constexpr const char* kCheckTexPath = "Asset/Textures/UI/Objective/Check.png";
	KdTexture m_checkTex;

	// 達成の記録(項目を出す前にやっても残る)
	std::shared_ptr<KdFontSprite>	   m_text[kKindCount];
	bool m_achieved[kKindCount] = {};
	std::vector<ObjectiveKind>		   m_items;
	std::weak_ptr<Magicwand>		   m_wpWand;
	std::weak_ptr<Player>			   m_wpPlayer;
	std::vector<std::weak_ptr<Switch>> m_wpSwitches;
	std::weak_ptr<WandPickup>		   m_wpPickup;

	std::shared_ptr<KdFontSprite> m_hintText;
	float			m_hintT			 = 1.0f;			// 表示の度合い(0=消えている 1=出ている)
	int				m_idleFrames	 = kHintIdleFrames;	// 止まっているフレーム数(最初から出すため満タン)
	Math::Vector3	m_prevPlayerPos;
	bool			m_hasPrevPos	 = false;			// 1フレーム目は比べる相手がないため
	int				m_holdFrames = 0;					// 出したままにする残りフレーム


	Phase m_phase = Phase::FadeIn;
	int   m_index = 0;	// 今出している項目の番号
	int   m_timer = 0;	// 今の段階に入ってからのフレーム数

	// 左からのスライドイン(オブジェクティブとTab：メニューの出入り)
	static constexpr float kSlideDist = 200.0f;			// 左から入ってくる距離
	static constexpr float kObjStep = 1.0f / 20.0f;	// 動きで出る・消える速さ(約0.33秒)
	float m_objT = 1.0f;	// オブジェクティブの表示の度合い(0=消えている 1=出ている)
};