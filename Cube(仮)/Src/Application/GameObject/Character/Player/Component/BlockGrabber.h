#pragma once

class NormalBlock;

class BlockGrabber
{
public:
	BlockGrabber() = default;
	~BlockGrabber() = default;

	// PlayerのUpdateから呼ばれる
	void Update(const Math::Vector3& playerPos, const Math::Matrix& playerRotMat);

	// 現在ブロックを持っているか？
	bool IsCarrying() const { return m_spCarriedBlock != nullptr; }

private:
	void HandleGrabAndDrop(const Math::Vector3& playerPos, const Math::Matrix& playerRotMat);
	void UpdateCarriedPos(const Math::Vector3& playerPos, const Math::Matrix& playerRotMat);
	void HandleDistanceControl(); // ★ ホイール入力で距離を変更する処理

	std::shared_ptr<NormalBlock> m_spCarriedBlock = nullptr;
	bool m_eKeyFlg = false;

	// ブロックとの保持距離（初期値 30.0f）
	float m_holdDistance = 30.0f;
};