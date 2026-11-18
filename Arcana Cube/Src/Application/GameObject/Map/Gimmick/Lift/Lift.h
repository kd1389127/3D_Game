#pragma once

// 往復して動く床。プレイヤーが乗ると一緒に動く
// (プレイヤーを動かす処理はPlayer側。ここは「今フレームに動いた量」を持つだけ)
class Lift : public KdGameObject
{
public:

	Lift() {}
	~Lift() override {}

	// 始点・終点・「止まらず動いている時間(秒)」を受け取る
	void Init(const Math::Vector3& startPos, const Math::Vector3& endPos, float seconds);
	void Update() override;
	void DrawLit() override;

	// 今フレームに動いた量(乗っているプレイヤーがこの分だけ一緒に動く)
	const Math::Vector3& GetDelta() const { return m_delta; }

	// 今のリフトの上面の高さ(Playerが「上面に当たったか」を調べるのに使う)
	float GetTopY() const { return m_curPos.y; }

private:

	std::shared_ptr<KdModelWork> m_spModel = nullptr;

	Math::Vector3 m_startPos = Math::Vector3::Zero;
	Math::Vector3 m_endPos = Math::Vector3::Zero;
	Math::Vector3 m_delta = Math::Vector3::Zero;

	float m_progress = -0.2f;						// 進捗度。-0.2から始めて、最初に少し止まる
	float m_speed = 0.0f;							// 1フレームの進み量
	bool  m_isReverse = false;						// trueなら戻っている途中
	Math::Vector3 m_curPos = Math::Vector3::Zero;	// 今のリフトの上面の位置(モデルの原点ではない)
	float m_topOffset = 0.0f;						// モデルの原点から上面までの高さ
	float m_endPause = 0.0f;						// 端で止まる分の進捗度(秒から変換した値)
};