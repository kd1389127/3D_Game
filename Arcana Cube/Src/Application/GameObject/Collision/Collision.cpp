#include "Collision.h"
#include "../../Scene/SceneManager.h"
#include "../Map/MapBlock/MapBlock.h"

namespace
{
	// 1マス=8の立方体は、中心から角まで約6.9。余裕を見て7
	constexpr float kBlockRadius = 7.0f;
}

void CollisionFilter::CollectAlongRay(const Math::Vector3& pos, const Math::Vector3& dir, float range,
	std::vector<KdGameObject*>& out, const KdGameObject* ignore)
{
	out.clear();
	const float radiusSq = kBlockRadius * kBlockRadius;

	for (auto& obj : SceneManager::Instance().GetObjList())
	{
		if (obj.get() == ignore) continue;

		// MapBlockだけ絞る。床・檻・背景は大きく、中心が遠くても体は近いので必ず入れる
		if (dynamic_cast<MapBlock*>(obj.get()))
		{
			// レイ(線分)の上で、ブロックの中心に一番近い点を内積で求める
			const Math::Vector3 center = obj->GetPos();
			const float t = std::clamp((center - pos).Dot(dir), 0.0f, range);
			const Math::Vector3 nearest = pos + dir * t;

			// 一番近い点でも7より遠ければ、このレイは当たりようがない
			if (Math::Vector3::DistanceSquared(center, nearest) > radiusSq) continue;
		}
		out.push_back(obj.get());
	}
}

void CollisionFilter::CollectAround(const Math::Vector3& center, float radius,
	std::vector<KdGameObject*>& out, const KdGameObject* ignore)
{
	out.clear();
	const float reach = radius + kBlockRadius;	// ブロック自体の大きさのぶん広げる

	for (auto& obj : SceneManager::Instance().GetObjList())
	{
		if (obj.get() == ignore) continue;

		if (dynamic_cast<MapBlock*>(obj.get()))
		{
			if (Math::Vector3::DistanceSquared(obj->GetPos(), center) > reach * reach) continue;
		}
		out.push_back(obj.get());
	}
}

bool IntersectRayAABB(const Math::Vector3& origin, const Math::Vector3& dir,
	const Math::Vector3& center, const Math::Vector3& halfExtents, float maxDist,
	float* tOut, Math::Vector3* normalOut) 
{
	Math::Vector3 boxMin = center - halfExtents;
	Math::Vector3 boxMax = center + halfExtents;

	float tMin = 0.0f;
	float tMax = maxDist;
	int   hitAxis = -1;
	float hitSign = 1.0f;

	const float* originArr = &origin.x;
	const float* dirArr = &dir.x;
	const float* boxMinArr = &boxMin.x;
	const float* boxMaxArr = &boxMax.x;

	for (int axis = 0; axis < 3; ++axis)
	{
		float o = originArr[axis];
		float d = dirArr[axis];
		float mn = boxMinArr[axis];
		float mx = boxMaxArr[axis];

		if (fabsf(d) < 1e-6f)
		{
			if (o < mn || o > mx) return false;
			continue;
		}

		float t1 = (mn - o) / d;
		float t2 = (mx - o) / d;
		if (t1 > t2) std::swap(t1, t2);

		if (t1 > tMin)
		{
			tMin = t1;
			hitAxis = axis;
			hitSign = (d > 0.0f) ? -1.0f : 1.0f; // 入った面の外向き法線の符号
		}
		tMax = std::min(tMax, t2);

		if (tMin > tMax) return false;
	}

	if (tOut) *tOut = tMin;
	if (normalOut)
	{
		*normalOut = Math::Vector3::Zero;
		if (hitAxis >= 0) (&normalOut->x)[hitAxis] = hitSign;
	}
	return true;
}
