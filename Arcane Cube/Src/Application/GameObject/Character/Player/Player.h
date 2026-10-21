#pragma once

class CameraBase;
class BlockGrabber;
class BlockDestroyer;
class KdSoundInstance;

struct CheckpointZone; 

class Player : public KdGameObject
{
public:
	Player();
	~Player();

	void Init(const Math::Vector3& startPos, float groundHeight, float startYawDeg,
		const std::vector<CheckpointZone>* pCheckpoints);
	void Update()	  override;
	void PostUpdate() override;

	// ブロックを保持しているかチェックする関数
	bool IsCarryingBlock()  const;
	bool IsGrabActive()		const;
	bool IsGrabTargetFar()  const;
	int  GetDropBlockedId() const;

	// ブロック削除関連
	bool HasDeleteTarget() const;
	bool TryDeleteTarget();

	// エイム中/プレビュー配置中かどうかをMagicwand側から知らせてもらう
	void SetAimBlocking(bool isBlocking) { m_isAimBlocking = isBlocking; }

	void Respawn();	// スタート位置・向きに戻る

	// リスポーンが起きたことを、他のクラス(Magicwand)に1回だけ伝える
	bool ConsumeRespawnedEvent()
	{
		const bool result = m_respawnedEvent;
		m_respawnedEvent = false;	// 読んだら下ろす(1回だけ伝えるため)
		return result;
	}

private:

	// 当たり判定
	void ResolveWallCollsion();
	void CheckGoal();
	// 隙間に入れる対策
	Math::Vector3 m_prevPos = Math::Vector3::Zero;
	bool m_prevHadHeadRoom = true;						// 前フレームは頭上が空いていたか
	static constexpr float kHeadRoomRayStart = 4.5f;	// ResolveWallCollsionのfootCut(4)より少し上から出す(階段の斜面に当たらないように)
	static constexpr float kHeadRoomRayOffset = 2.5f;	// 体の半径(3)より少し内側(壁際で誤反応しないように)

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

	// ブロック操作コンポーネント（所有権管理）
	std::unique_ptr<BlockGrabber> m_upBlockGrabber = nullptr;
	std::unique_ptr<BlockDestroyer>m_upBlockDestroyer = nullptr;

	// ワールド座標
	Math::Vector3 m_pos = Math::Vector3::Zero;

	// スポーン地点
	Math::Vector3 m_startPos = Math::Vector3::Zero;

	// 落ちた時に戻る場所(最初はスタート位置。中間地点の箱に入るたびに書き換わる)
	Math::Vector3 m_respawnPos = Math::Vector3::Zero;
	Math::Vector3 m_respawnDegAng = Math::Vector3::Zero;
	const std::vector<CheckpointZone>* m_pCheckpoints = nullptr;

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
	const float m_jumpPow = 0.9f;
	bool m_isGrounded = false;		// 今、地面に立っているか(PostUpdateで更新)

	bool m_isAimBlocking = false;
	bool m_respawnedEvent = false;

	// SE
	std::shared_ptr<KdSoundInstance> m_spWalkSe;	// 歩いている間のループ音
	bool m_goalSePlayed = false;	// ゴール音をもう鳴らしたか
};