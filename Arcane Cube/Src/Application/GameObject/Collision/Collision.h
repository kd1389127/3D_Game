#pragma once

// 判定する相手を、近いものだけに絞るための関数群
namespace CollisionFilter
{
	// レイが通る道の近くにあるものを out に集める(dirは長さ1であること)
	void CollectAlongRay(const Math::Vector3& pos, const Math::Vector3& dir, float range,
		std::vector<KdGameObject*>& out, const KdGameObject* ignore = nullptr);

	// 指定した中心の周りにあるものを out に集める
	void CollectAround(const Math::Vector3& center, float radius,
		std::vector<KdGameObject*>& out, const KdGameObject* ignore = nullptr);
}