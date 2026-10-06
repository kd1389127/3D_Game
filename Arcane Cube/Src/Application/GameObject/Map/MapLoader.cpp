#include "MapLoader.h"
#include "../Map/MapBlock/MapBlock.h"
#include "../Block/BlockGridManager.h"

bool MapLoader::Load(const std::string& csvPath, float groundHeight, int originCol,
	int originRow, std::vector<std::shared_ptr<KdGameObject>>& outObject)
{
	KdCSVData csv(csvPath);
	const auto& lines = csv.GetLData();
	if (lines.empty()) return false;

	// ------------------------------------------------------------
	// ① CSV → m_grid
	//   「#0」「#1」…の行が新しい層の開始、それ以外が1行ぶんのセル列
	// ------------------------------------------------------------
	m_grid.clear();

	for (const auto& line : lines)
	{
		if (line.empty() || line[0].empty())continue;

		// 先頭セルのBOM(EF BB BF)を取り除く
		std::string first = line[0];
		if (first.size() >= 3 &&
			(unsigned char)first[0] == 0xEF &&
			(unsigned char)first[1] == 0xBB &&
			(unsigned char)first[2] == 0xBF)
		{
			first.erase(0, 3);
		}

		// 「#0」「#1」…は新しい層の開始
		if (!first.empty() && first[0] == '#')
		{
			m_grid.emplace_back();
			continue;
		}
		
		if (m_grid.empty()) m_grid.emplace_back();

		std::vector<int> row;
		row.reserve(line.size());
		for (const auto& s : line)
		{
			row.push_back(s.empty() ? 0 : std::atoi(s.c_str()));
		}
		m_grid.back().push_back(std::move(row));
	}

	if (m_grid.empty() || m_grid[0].empty())return false;

	// 読めているかの確認ログ(Map1なら layers=9 rows=10 cols=29 になるはず)
	KdDebugGUI::Instance().AddLog("[MapLoader] layers=%d rows=%d cols=%d\n",
		(int)m_grid.size(), (int)m_grid[0].size(), (int)m_grid[0][0].size());

	// ------------------------------------------------------------
	// ② セルごとに生成する
	//   層0=床。層0の上面がgroundHeightになる
	// ------------------------------------------------------------
	constexpr float G = BlockGridManager::GridSize;
	int spawned = 0, skipped = 0, unknown = 0;

	for (int y = 0; y < (int)m_grid.size(); ++y)
	{
		for (int z = 0; z < (int)m_grid[y].size(); ++z)
		{
			for (int x = 0; x < (int)m_grid[y][z].size(); ++x)
			{
				const int type = m_grid[y][z][x];
				if (type == Empty)continue;

				// CSVの値 → 形と向き(知らない値は警告を出して飛ばす)
				MapBlock::Shape shape = MapBlock::Shape::Full;
				MapBlock::StairDir dir = MapBlock::StairDir::PosX;

				switch (type)
				{
				case Normal:                                                          
					break;
				case StairPosX: shape = MapBlock::Shape::Stair; dir = MapBlock::StairDir::PosX; 
					break;
				case StairNegX: shape = MapBlock::Shape::Stair; dir = MapBlock::StairDir::NegX; 
					break;
				case StairPosZ: shape = MapBlock::Shape::Stair; dir = MapBlock::StairDir::PosZ; 
					break;
				case StairNegZ: shape = MapBlock::Shape::Stair; dir = MapBlock::StairDir::NegZ; 
					break;
				default:
					KdDebugGUI::Instance().AddLog("[MapLoader] 不明な値 %d (層%d 行%d 列%d)\n", type, y, z, x);
					++unknown;
					continue;
				}

				// セルの底面のY(層０の上面 = groundHeight)
				const float cellBottom = groundHeight + (y - 1) * G;

				// セル中心(BlockGridManagerのセル中心と一致させる)
				const Math::Vector3 cellCenter
				(
					(x - originCol) * G,
					cellBottom + G * 0.5f,
					(z - originRow) * G
				);

				// 全セルを登録する(見えない埋まりセルも。魔法ブロックの重複防止に必要)
				BlockGridManager::Instance().Register(cellCenter, BlockGridManager::BlockKind::Map);

				// 6方向をフルブロックで囲まれていたら生成しない(描画・当たり判定の節約)
				if (shape == MapBlock::Shape::Full && IsBuried(x, y, z)) { ++skipped; continue; }

				auto spBlock = std::make_shared<MapBlock>();
				spBlock->Init(cellCenter, shape,dir);
				outObject.push_back(spBlock);
				++spawned;
			}
		}
	}
	KdDebugGUI::Instance().AddLog("[MapLoader] spawned=%d skipped(buried)=%d unknown=%d\n", spawned, skipped, unknown);
	return true;
}

// 範囲外・空セルは0(空)として扱う
int MapLoader::GetCell(int x, int y, int z) const
{
	if (y < 0 || y >= (int)m_grid.size())		 return Empty;
	if (z < 0 || z >= (int)m_grid[y].size())	 return Empty;
	if (x < 0 || x >= (int)m_grid[y][z].size())  return Empty;
	return m_grid[y][z][x];
}

// ハーフは面を完全には塞がないので、埋まり判定にはフルブロックだけを使う
bool MapLoader::IsBuried(int x, int y, int z) const
{
	return IsFull(x + 1, y, z) && IsFull(x - 1, y, z)
		&& IsFull(x, y + 1, z) && IsFull(x, y - 1, z)
		&& IsFull(x, y, z + 1) && IsFull(x, y, z - 1);
}
