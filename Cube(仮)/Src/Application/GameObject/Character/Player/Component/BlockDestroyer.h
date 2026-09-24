#pragma once

class NormalBlock;

class BlockDestroyer
{
public:
	BlockDestroyer()  = default;
	~BlockDestroyer() = default;

	// PlayerのUpdateから呼ばれる
	void Update(const Math::Vector3& playerPos, const Math::Matrix& playerRotMat);

	// 削除モード中かどうか
	bool IsDeleteMode() const { return m_isDeleteMode; }

private:

	void HandleModeToggle();

	// レイキャストで、レティクルが合っている確定済みブロックを探してハイライトを切り替える
	void UpdateTargeting(const Math::Vector3& playerPos, const Math::Matrix& playerRotMat);
	
	void HandleDeleteInput();
	void ClearTarget();

	bool m_isDeleteMode = false;
	bool m_qKeyFlg = false;
	bool m_leftDownPrev = false;

	// 今ハイライト中の「スタック全体」(単体の時は要素1つ)
	std::vector<std::weak_ptr<NormalBlock>> m_wpTargetStack;
};