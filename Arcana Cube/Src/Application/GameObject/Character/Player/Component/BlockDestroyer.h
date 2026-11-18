#pragma once

class NormalBlock;

class BlockDestroyer
{
public:
	BlockDestroyer()  = default;
	~BlockDestroyer() = default;

	// PlayerのUpdateから呼ばれる
	void Update(const Math::Vector3& playerPos, const Math::Matrix& playerRotMat);

	// 現在、削除対象を狙っているか
	bool HasTarget() const { return !m_wpTarget.expired(); }

	// 狙っているブロックを1個だけ削除し、魔力を1回復する。対象が無ければ何もせずfalseを返す
	bool TryDeleteTarget();

	// 対象のハイライトを消す(エイム中など、削除判定自体を止めたい時に呼ぶ)
	void ClearTarget();

private:

	// レイキャストで、レティクルが合っている確定済みブロックを探してハイライトを切り替える
	void UpdateTargeting(const Math::Vector3& playerPos, const Math::Matrix& playerRotMat);

	// 今ハイライト中のブロック
	std::weak_ptr<NormalBlock> m_wpTarget;
};