#pragma once

// 杖の魔法石から、掴んでいるブロックへ走るプラズマ(稲妻)
// Magicwandが持つ部品。描画だけを担当し、位置は毎フレーム教えてもらう
class GrabBeam
{
public:
	GrabBeam() = default;
	~GrabBeam() = default;

	void Init();

	// 毎フレーム呼ぶ
	//  isActive : 掴んでいる間true
	//  startPos : 魔法石の位置　endPos : ブロックの中心　eyePos : カメラ(目)の位置
	void Update(bool isActive, const Math::Vector3& startPos, const Math::Vector3& endPos, const Math::Vector3& eyePos);

	void Draw();

private:
	void RefreshOffsets();
	void BuildBoltPoints(std::vector<Math::Vector3>& outPoints) const;
	void DrawLayer(const std::vector<Math::Vector3>& points, float widthPerDist, const Math::Color& color);
	std::shared_ptr<KdSquarePolygon> m_spPolygon;	// 光の帯の画像を貼った板(1枚を使い回す)
	std::vector<Math::Vector2> m_offsets;			// 各点の横ずれ(-1〜1)。数フレームに1回作り直す
	int   m_frame = 0;								// 作り直しのタイミングを数える
	float m_rate = 0.0f;							// 出具合(0〜1)。掴むと増え、離すと減る

	Math::Vector3 m_startPos = Math::Vector3::Zero;
	Math::Vector3 m_endPos = Math::Vector3::Zero;	// 離した後の消えていく間も、最後の位置を使う
	Math::Vector3 m_eyePos = Math::Vector3::Zero;
};