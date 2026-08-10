#pragma once
#include <unordered_set>
#include <cmath>

class BlockGridManager
{
public:
	static constexpr float GridSize = 8.0f; // ブロック1個分のサイズに合わせて調整

	static BlockGridManager& Instance()
	{
		static BlockGridManager inst;
		return inst;
	}

	static Math::Vector3 SnapToGrid(const Math::Vector3& pos)
	{
		return Math::Vector3
		(
			std::round(pos.x / GridSize) * GridSize,
			std::round(pos.y / GridSize) * GridSize,
			std::round(pos.z / GridSize) * GridSize
		);
	}

	  bool IsOccupied(const Math::Vector3& snappedPos) const
    {
        return m_occupied.count(ToKey(snappedPos)) > 0;
    }

	void Register(const Math::Vector3& snappedPos)
	{
		m_occupied.insert(ToKey(snappedPos));
	}

	static Math::Vector3 SnapNormalToAxis(const Math::Vector3& normal)
	{
		float ax = fabsf(normal.x), ay = fabsf(normal.y), az = fabsf(normal.z);

		if (ax >= ay && ax >= az) return Math::Vector3(normal.x > 0 ? 1.f : -1.f, 0, 0);
		if (ay >= ax && ay >= az) return Math::Vector3(0, normal.y > 0 ? 1.f : -1.f, 0);
		return Math::Vector3(0, 0, normal.z > 0 ? 1.f : -1.f);
	}

private:
	static std::tuple<int, int, int> ToKey(const Math::Vector3& pos)
	{
		return
		{
			(int)std::round(pos.x / GridSize),
			(int)std::round(pos.y / GridSize),
			(int)std::round(pos.z / GridSize)
		};
	}

	struct KeyHash
	{
		size_t operator()(const std::tuple<int, int, int>& k) const
		{
			auto [x, y, z] = k;
			return std::hash<int>()(x) ^ (std::hash<int>()(y) << 1) ^ (std::hash<int>()(z) << 2);
		}
	};

	std::unordered_set<std::tuple<int, int, int>, KeyHash> m_occupied;
};