#pragma once
#include <wrl/client.h>

class NormalBlock;
class BlockGridManager;
class IGrabbable;

class BlockGrabber
{
public:
	BlockGrabber() = default;
	~BlockGrabber() = default;

	void Init(float groundHeight = 0.0f);

	// PlayerのUpdateから呼ばれる
	void Update(const Math::Vector3& playerPos, const Math::Matrix& playerRotMat);

	// 現在ブロックを持っているか？
	bool IsCarrying() const { return m_spCarriedBlock != nullptr; }

	float GetMinY() const;

	// 現在持っているブロックの座標を取得（持っていなければ判定不要なので呼び出し側でIsCarrying()チェック）
	Math::Vector3 GetCarriedBlockPos() const;
	
	// レティクルを緑にするか(ギミックブロックを運んでいる、または狙っている)
	bool IsGrabActive() const { return m_spCarriedBlock != nullptr || m_hasGrabTarget; }

private:
	void HandleGrabAndDrop(const Math::Vector3& playerPos, const Math::Matrix& playerRotMat);
	void UpdateCarriedPos(const Math::Vector3& playerPos, const Math::Matrix& playerRotMat);

	Math::Vector3 CalcTargetPos(const Math::Vector3& playerPos, const Math::Matrix& playerRotMat) const;

	// 視線の先にある、掴めるギミックブロックを探す(無ければnullptr)
	std::shared_ptr<KdGameObject> FindGradTarget(const Math::Vector3& playerPos, const Math::Matrix& playerRotMat)const;
	void UpdateGrabTarget(const Math::Vector3& playerPos, const Math::Matrix& playerRotMat);
	
	// ハイライトの対象を切り替える(前の対象は消し、新しい対象を光らせる)
	void SetGrabHighlightTarget(const std::shared_ptr<KdGameObject>& target);
	// 今ハイライト中のブロック
	std::weak_ptr<KdGameObject> m_wpHighlighted;

	std::shared_ptr<KdGameObject> m_spCarriedBlock = nullptr;

	bool m_eKeyFlg = false;

	// ブロックとの保持距離（初期値 30.0f）
	float m_holdDistance = 30.0f;

	float m_groundHeight = 0.0f;

	bool m_hasGrabTarget = false;
};