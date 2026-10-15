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