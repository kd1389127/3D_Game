#pragma once

// 1ステージ分の配置情報
struct StageData
{
	Math::Vector3 playerStartPos;		 // プレイヤーの初期座標
	Math::Vector3 goalPos;				 // ゴールの座標
	float         groundHeight = 0.0f;   // ブロックの下限に使う、そのステージの地面のY GetMinY()が0.0f + 4.0f = 4.0f
	float         playerStartYaw = 0.0f; // プレイヤーの開始時の向き(度)。0=+Z方向

	//==============================================================================
	// スイッチ・檻(鉄格子)・ギミック専用ブロックの配置(このステージで使う場合のみ)
	//==============================================================================
	bool          hasGimmickCage = false;					// このステージにスイッチ/檻/ギミックブロックを配置するか
	Math::Vector3 switchPos = Math::Vector3::Zero;			// スイッチの座標
	Math::Vector3 cageBasePos = Math::Vector3::Zero;		// 檻(鉄格子)の基準座標(閉じている時の位置)
	Math::Vector3 gimmickBlockPos = Math::Vector3::Zero;	// ギミック専用ブロックの初期配置座標

	//==============
	// CSV方式のMap
	//==============
	std::string   mapCsvPath;
	int           csvOriginCol = 0;	// CSV上でワールド原点(X=0)にあたる列
	int           csvOriginRow = 0;	// CSV上でワールド原点(Z=0)にあたる行
	
};

// ステージテーブル(ステージを増やす場合はここに追加していくだけでOK)
static const StageData g_stageTable[] =
{
	// ステージ1
	// プレイヤー開始位置、ゴールの位置、ステージの下限、プレイヤーの向き
	// ギミックを使うか、ギミックの各配置、CSVデータ、CSVの列、CSVの行
	{ 
		Math::Vector3(80, 0, 10), Math::Vector3(-100, 25, 10), 0.0f,-90.0f,
		false, Math::Vector3::Zero, Math::Vector3::Zero, Math::Vector3::Zero,
		"Asset/Data/MapData/Map1.csv", 14, 4 
	},

	// ステージ2(スイッチ・檻・ギミックブロックの座標)
	{
		Math::Vector3(48, 0, 0), Math::Vector3(-64, 33, 0), 0.0f,-90.0f,
		true,
		Math::Vector3(0, 0, -16),			// switchPos
		Math::Vector3(-48, 24.0f, 0),		// cageBasePos
		Math::Vector3(64, 52.0f, 0),		// gimmickBlockPos
		"Asset/Data/MapData/Map2.csv", 14, 5 
	},
	
	//// ステージ３(仮の値。後で調整してください)
	//{ Math::Vector3(0, 0, 0),      Math::Vector3(0, 0, 0) },
};

static const int g_stageCount = sizeof(g_stageTable) / sizeof(g_stageTable[0]);