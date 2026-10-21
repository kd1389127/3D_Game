#pragma once

// 操作説明のUIを出すクラス。Tabアイコンは常時、操作バナーはMap1だけ出す
// ステージ開始で一度出て、一定時間で消える。プレイヤーが止まったままだと、また出る
class Guide : public KdGameObject
{
public:
	Guide() {}
	~Guide()	override {}

	void Init(bool showBanner);   
	void Update()		override;
	void DrawSprite()	override;

	// 止まっているかを調べる相手(プレイヤー)を渡してもらう
	void SetTarget(const std::shared_ptr<KdGameObject>& target) { m_wpTarget = target; }

private:
	static constexpr int   kShowFrames = 60 * 8;		// 1回出している長さ(約8秒)
	static constexpr int   kIdleFrames = 60 * 10;		// 止まってから、また出るまで(約10秒)
	static constexpr float kFadeStep = 1.0f / 20.0f;	// 出る・消える速さ
	static constexpr float kMoveEps = 0.0001f;			// これ以下の移動は「止まっている」とみなす

	// Tabアイコン(画面右上、常時表示)
	static constexpr float kTabW = 84.0f;
	static constexpr float kTabH = 56.0f;
	static constexpr float kTabAlpha = 0.75f;	// 少し透かして、目立ちすぎないようにする
	static constexpr float kTabPosX = 640.0f - 24.0f - kTabW / 2;
	static constexpr float kTabPosY = 360.0f - 20.0f - kTabH / 2;

	// 画面左上に出す。画面の中心が(0,0)・上がプラス。1280x720の左上から、横24・縦20あけた位置
	// 画像(302x334)の縦横比を保った大きさ
	static constexpr float kW = 240.0f;
	static constexpr float kH = kW * 414.0f / 302.0f;	// 約329。画像の縦横比のまま
	static constexpr float kPosX = -640.0f + 24.0f + kW / 2;
	static constexpr float kPosY = 360.0f - 20.0f - kH / 2;

	KdTexture m_tex;
	std::weak_ptr<KdGameObject> m_wpTarget;

	KdTexture m_tabTex;
	bool m_showBanner = false;	// 操作説明のバナーを出すか(Map1だけ)

	Math::Vector3 m_prevPos = Math::Vector3::Zero;	// 前フレームのプレイヤー位置
	int   m_showTimer = 0;		// 0より大きい間は表示する
	int   m_idleTimer = 0;		// 消えている間に、止まっていたフレーム数
	float m_alpha = 0.0f;		// 今の透明度(0〜1)
};