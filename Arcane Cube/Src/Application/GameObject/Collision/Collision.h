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

// レイ(origin, dir)が、center±halfExtentsのAABBに当たるか判定する(スラブ法)
// 当たった場合、出会うまでの距離(tOut)と、入った面の法線(normalOut)を返す
bool IntersectRayAABB(const Math::Vector3& origin, const Math::Vector3& dir,
	const Math::Vector3& center, const Math::Vector3& halfExtents, float maxDist,
	float* tOut = nullptr, Math::Vector3* normalOut = nullptr);
