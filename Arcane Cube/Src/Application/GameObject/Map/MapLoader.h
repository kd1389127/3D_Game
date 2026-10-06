#pragma once

class MapBlock;

// CSVからMap用ブロックを生成するクラス
class MapLoader
{
public:
	// csvPath       … Map用CSV
	// groundHeight  … そのステージのBlockGridManagerの地面の高さ(層0=床の上面のY)
	// originCol/Row … CSV上のどの列・行がワールド原点(X=0, Z=0)か
	// outObjects    … 生成したブロックがここに追加される(シーンへの登録は呼び出し側で行う)

	bool Load(const std::string& csvPath, float groundHeight, int originCol, int originRow,
		std::vector<std::shared_ptr<KdGameObject>>& outObject);

private:
	enum CellType 
	{
		Empty = 0,		// 何もなし
		Normal = 1,		// Normalのブロック
		StairNegX = 2,	// -X方向に上る階段
		StairPosX = 3,	// +X方向に上る階段
		StairNegZ = 4,	// -Z方向(行が減る方)に上る階段
		StairPosZ = 5,	// +Z方向(行が増える方)に上る階段
	};

	// m_grid[層y][行z][列x]
	std::vector<std::vector<std::vector<int>>> m_grid;

	int GetCell(int x, int y, int z) const;
	bool IsFull(int x, int y, int z) const { return GetCell(x, y, z) == Normal; }
	bool IsBuried(int x, int y, int z) const;

};