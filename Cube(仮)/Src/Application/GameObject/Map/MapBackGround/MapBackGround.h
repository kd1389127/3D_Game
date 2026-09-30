#pragma once

class MapBackGround : public KdGameObject
{
public:
	MapBackGround() {};
	~MapBackGround() 	override {};

	void Init()			override;
	void Update()		override;
	void DrawUnLit()	override;

	// 追従する対象(プレイヤー)
	void SetTarget(const std::shared_ptr<KdGameObject>& target) { m_wpTarget = target; }
	void SetBaseY(float y) { m_baseY = y; }

private:

	std::shared_ptr<KdModelData> m_spModel = nullptr;
	std::weak_ptr<KdGameObject>  m_wpTarget;

	float m_scale = 100.0f;	// ドームの大きさ(見ながら調整する)
	float m_baseY = 0.0f;	// ドームのY位置(地平線の高さ合わせ用)
	float m_yawDeg = 185.0f;	// ドームのY軸回転(度)。綺麗な景色が進行方向に来るよう調整する
};