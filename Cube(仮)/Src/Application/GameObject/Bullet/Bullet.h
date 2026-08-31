#pragma once

class Bullet : public KdGameObject
{
public:
	Bullet() {}
	~Bullet()		override {}

	// 発射開始位置と、着弾する目標位置を渡して初期化
	//
	// onImpact … 着弾した瞬間に呼ばれるコールバック関数
	//             渡した場合：Bullet自身はブロックを作らず、着弾点(hitPos)と
	//                         軸方向に丸めた法線(axisNormal)を渡して呼び出し元に処理を任せる
	//                        (今回はMagicwand::EnterAdjustModeに繋がっている)
	//             nullptrのまま … 従来通りBullet自身がその場でNormalBlockを即生成する(フォールバック)
	void Init(const Math::Vector3& startPos, const Math::Vector3& targetPos, const Math::Vector3& hitNormal,
		std::function<void(const Math::Vector3& hitPos, const Math::Vector3& axisNormal)> onImpact = nullptr);

	void Update()	override;
	void DrawUnLit()	override;
	void DrawBright()	override;

private:
	std::shared_ptr<KdModelWork> m_spModel = nullptr;

	Math::Vector3 m_targetPos = Math::Vector3::Zero;	// 着弾予定地点
	Math::Vector3 m_dir = Math::Vector3::Zero;			// 発射方向(単位ベクトル)
	Math::Vector3 m_hitNormal = Math::Vector3::Up;		// 着弾面の法線(まだ軸に丸める前の生の値)

	// 着弾時に呼び出すコールバック(Magicwandなど、生成側の処理を注入するための仕組み)
	std::function<void(const Math::Vector3&, const Math::Vector3&)> m_onImpact = nullptr;

	const float m_speed = 2.0f;							// 弾の移動速度

};