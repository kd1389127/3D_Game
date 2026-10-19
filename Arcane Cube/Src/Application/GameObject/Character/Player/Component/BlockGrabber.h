#pragma once

class IGrabbable;
class KdEffekseerObject;
class KdSoundInstance;

class BlockGrabber
{
public:
	BlockGrabber() = default;
	~BlockGrabber();

	void Init(float groundHeight = 0.0f);

	// PlayerのUpdateから呼ばれる
	void Update(const Math::Vector3& playerPos, const Math::Matrix& playerRotMat);

	// 現在ブロックを持っているか？
	bool IsCarrying() const { return m_spCarriedBlock != nullptr; }
	float GetMinY() const;

	// 現在持っているブロックの座標を取得（持っていなければ判定不要なので呼び出し側でIsCarrying()チェック）
	Math::Vector3 GetCarriedBlockPos() const;
	
	// 届く距離で狙っている
	bool IsGrabActive() const { return m_spCarriedBlock != nullptr || m_hasGrabTarget; }

	// ギミックブロックを狙っているが、遠くて届かない
	bool IsGrabTargetFar() const { return m_isTargetFar; }

	//「離せなかった」回数。増えたらレティクルが震える(MagicManagerの失敗IDと同じ発想)
	int GetDropBlockedId() const { return m_dropBlockedId; }

	// 掴んでいるブロック(プレイヤーの足場にしないために使う。持っていなければnullptr)
	const KdGameObject* GetCarriedBlock() const { return m_spCarriedBlock.get(); }

	// 持っているブロックを手放して、元の場所に戻す(奈落に落ちた時用)。持っていなければ何もしない
	void ReleaseAndReturn();

private:
	void HandleGrabAndDrop(const Math::Vector3& playerPos, const Math::Matrix& playerRotMat);
	void UpdateCarriedPos(const Math::Vector3& playerPos, const Math::Matrix& playerRotMat);
	Math::Vector3 CalcTargetPos(const Math::Vector3& playerPos, const Math::Matrix& playerRotMat) const;

	//毎フレーム、狙っているギミックブロックの状態(届く / 遠い)を更新する
	void UpdateGrabTarget(const Math::Vector3& playerPos, const Math::Matrix& playerRotMat);
	// ハイライトの対象を切り替える(前の対象は消し、新しい対象を光らせる)
	void SetGrabHighlightTarget(const std::shared_ptr<KdGameObject>& target);
	// 今ハイライト中のブロック
	std::weak_ptr<KdGameObject> m_wpHighlighted;

	// 掴んでいる間のエフェクト
	void StartGrabEffect();
	void UpdateGrabEffect();
	void StopGrabEffect();
	std::weak_ptr<KdEffekseerObject> m_wpGrabEffect;

	// 掴み中のループ音(止める時に使うので持っておく)
	std::shared_ptr<KdSoundInstance> m_spHoldSe;	

	std::shared_ptr<KdGameObject> m_spCarriedBlock = nullptr;
	bool m_eKeyFlg = false;
	float m_holdDistance = 30.0f;	// ブロックとの保持距離（初期値 30.0f）
	float m_groundHeight = 0.0f;

	bool m_hasGrabTarget = false;	// 届く距離で狙っている
	bool m_isTargetFar = false;		// 狙っているが遠い
	int  m_dropBlockedId = 0;		// 離せなかった回数
};