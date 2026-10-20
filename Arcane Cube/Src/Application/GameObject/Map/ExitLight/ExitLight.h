#pragma once

// 終盤のステージの突き当たりにある「外の光」(板ポリゴン)
class ExitLight : public KdGameObject
{
public:
	ExitLight() {}
	~ExitLight()	override {}

	void Init(const Math::Vector3& pos);
	void DrawUnLit()	override;

private:
	static constexpr float kWidth = 24.0f;		// 通路の幅(3マス)に合わせる
	static constexpr float kHeight = 24.0f;
	static constexpr float kBrightness = 1.2f;	// 暗い通路で光って見えるよう、少し明るくする

	std::shared_ptr<KdSquarePolygon> m_spPolygon;
};