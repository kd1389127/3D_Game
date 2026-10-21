#pragma once

// 終盤のステージで、階段を上るにつれて画面を白くしていくクラス
// 真っ白になったら、白いフェードでリザルトへ移る
class ExitLight : public KdGameObject
{
public:
	ExitLight() {}
	~ExitLight()	override {}

	void Init(const Math::Vector3& pos);
	void Update()		override;
	void DrawSprite()	override;
	void DrawUnLit()	override;
	void DrawBright()	override;

	// 上った位置を調べる相手(プレイヤー)を渡してもらう
	void SetTarget(const std::shared_ptr<KdGameObject>& target) { m_wpTarget = target; }

private:
	
	static constexpr float kStairStartX = -24.0f;	// = (5 - originCol) * 8。階段の半ばから白くなり始める
	static constexpr float kFullWhiteX  = -52.0f;	// = 壁の手前面(-60) + 8。光の手前で真っ白

	static constexpr float kGlowW = 24.0f;			// 通路の幅(3マス)
	static constexpr float kGlowH = 32.0f;			// 出口の高さ(4層分)
	static constexpr float kGlowBright = 3.0f;		// 1より大きくして、眩しい白にする
	std::shared_ptr<KdSquarePolygon> m_spPolygon;

	std::weak_ptr<KdGameObject> m_wpTarget;	// プレイヤー
	float m_rate = 0.0f;					// 白さの進み具合(0〜1)
	bool  m_triggered = false;				// もうリザルトへ向かい始めたか
};