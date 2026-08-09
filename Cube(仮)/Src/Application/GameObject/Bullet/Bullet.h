#pragma once

class Bullet : public KdGameObject
{
public:
	Bullet(){}
	~Bullet()		override{}

	// 発射開始位置と、着弾する目標位置を渡して初期化
	void Init(const Math::Vector3& startPos, const Math::Vector3& targetPos);

	void Update()	override;
	void DrawLit()	override;

private:
	std::shared_ptr<KdModelWork> m_spModel = nullptr;

	Math::Vector3 m_targetPos	= Math::Vector3::Zero;
	Math::Vector3 m_dir			= Math::Vector3::Zero;

	const float m_speed = 2.0f;			// 弾の移動速度

};