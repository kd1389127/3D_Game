#pragma once

class CameraBase;
class BlockGrabber;

class Player : public KdGameObject
{
public:
	Player();
	~Player() override;

	void Init(const Math::Vector3& startPos);
	void Update()	  override;
	void PostUpdate() override;
	void DrawLit()	  override;

	// ブロックを保持しているかチェックする関数
	bool IsCarryingBlock() const;

private:

	// 当たり判定
	void ResolveWallCollsion();
	void CheckGoal();

	// 持っているブロックの中に入り込んでいたら押し出す
	void ResolvePushOutFromCarriedBlock();

	// カメラ情報
	void UpdateRotateByMouse();

	const Math::Matrix GetRotationMatrix()const
	{
		return Math::Matrix::CreateFromYawPitchRoll(
			DirectX::XMConvertToRadians(m_degAng.y),
			DirectX::XMConvertToRadians(m_degAng.x),
			DirectX::XMConvertToRadians(m_degAng.z));
	}

	const Math::Matrix GetRotationYMatrix() const
	{
		return Math::Matrix::CreateRotationY(
			DirectX::XMConvertToRadians(m_degAng.y));
	}

	Math::Vector3 m_degAng = Math::Vector3::Zero;
	// カメラ回転用マウス座標の差分
	POINT m_fixMousePos = { 640,360 };

	// ★ ブロック操作コンポーネント（所有権管理）
	std::unique_ptr<BlockGrabber> m_upBlockGrabber = nullptr;

	// ワールド座標
	Math::Vector3 m_pos = Math::Vector3::Zero;

	// 進行方向(ベクトルの向き)
	Math::Vector3 m_moveDir = Math::Vector3::Zero;

	// 移動速度
	const float m_moveSpeed = 0.5f;

	// プレイヤーの座標補正
	const float m_adjustHeight = 10.0f;

	// 重力
	const float m_gravityPow = 0.04f;
	float m_gravity = 0.0f;

	// キーフラグ
	bool m_jumpKeyFlg = false;

	// ジャンプ力
	const float m_jumpPow = 1.0f;

};