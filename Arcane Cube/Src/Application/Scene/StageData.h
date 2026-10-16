#pragma once

// 1台のリフトの配置情報
struct LiftData
{
	Math::Vector3 startPos = Math::Vector3::Zero;	// 始点(リフトの原点の位置)
	Math::Vector3 endPos = Math::Vector3::Zero;		// 終点
	float         seconds = 8.0f;					// 止まらず動いている時間(秒)
};

// 檻1つ分の追加配置(cageBasePosの檻に加えて、同じスイッチで一緒に開く)
struct CageData
{
	Math::Vector3 pos = Math::Vector3::Zero;
	float yaw = 90.0f;
};

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
	bool          hasGimmickCage  = false;					// このステージにスイッチ/檻/ギミックブロックを配置するか
	std::vector<Math::Vector3>		switchPositions;		// スイッチの座標(複数可)
	Math::Vector3 cageBasePos	  = Math::Vector3::Zero;	// 檻(鉄格子)の基準座標(閉じている時の位置)
	std::vector<Math::Vector3>		gimmickBlockPositions;	// ギミック専用ブロックの初期座標(複数可)
	
	//==============
	// CSV方式のMap
	//==============
	std::string   mapCsvPath;
	int           csvOriginCol = 0;	// CSV上でワールド原点(X=0)にあたる列
	int           csvOriginRow = 0;	// CSV上でワールド原点(Z=0)にあたる行
	int			  buildLimitLayers = 0;

	float         cageHeight = 32.0f;								// 檻が開く高さ

	int           tallBuildLimitLayers = 0;							// 高く積めるエリアの上限層数(0ならなし)
	Math::Vector2 tallZoneMin = Math::Vector2::Zero;				// エリアのXZ最小(yにはワールドZが入る)
	Math::Vector2 tallZoneMax = Math::Vector2::Zero;				// エリアのXZ最大

	Math::Vector3 cageScale = Math::Vector3(55.0f, 45.0f, 45.0f);	// 檻の大きさ
	std::vector<LiftData> lifts;									// リフト(なければ空)
	std::vector<CageData> extraCages;
};

// ステージテーブル(ステージを増やす場合はここに追加していくだけでOK)
static const StageData g_stageTable[] =
{
	// ステージ1
	// プレイヤー開始位置、ゴールの位置、ステージの下限、プレイヤーの向き
	// ギミックを使うか、ギミックの各配置、CSVデータ、CSVの列、CSVの行、置ける最大の上面
	{ 
		Math::Vector3(80, 0, 10), Math::Vector3(-100, 13.5, 10), 0.0f,-90.0f,
		false, {}, Math::Vector3::Zero,{},
		"Asset/Data/MapData/Map1.csv", 14, 4 
	},

	// ステージ2(スイッチ・檻・ギミックブロックの座標)
	{
		Math::Vector3(48, 0, 0), Math::Vector3(-60, 21.5, 0), 0.0f,-90.0f,
		true,
		{Math::Vector3(0, 0, -16)},			// switchPositions
		Math::Vector3(-48, 24.0f, 0),		// cageBasePos
		{Math::Vector3(64, 52.0f, 0)},		// gimmickBlockPositions
		"Asset/Data/MapData/Map2.csv", 14, 5 
	},
	
	// ステージ3：奈落を橋で渡り、塔の壁に柱を積んで登ってゴールへ
	// ステージ3：リフトの紹介。ブロックを持ってリフトで奈落を渡り、スイッチに置いて檻を開ける
	{
		Math::Vector3(0, 0, 0), Math::Vector3(208, -2.5f, 0), 0.0f, 90.0f,
		true,
		{ Math::Vector3(176, 0, 0) },					// switchPositions
		Math::Vector3(192, 0, 0),						// cageBasePos
		{ Math::Vector3(18, 4, 16) },					// gimmickBlockPositions
		"Asset/Data/MapData/Map3.csv", 2, 4,			// originCol=2, originRow=4
		2,												// buildLimitLayers
		32.0f,											// cageHeight
		0, Math::Vector2::Zero, Math::Vector2::Zero,	// 高く積めるエリアなし
		Math::Vector3(55.0f, 40.0f, 45.0f),				// cageScale
		{
		// 奈落は X=36〜148。リフトの横幅が24なら、始点=36+12、終点=148-12
		{ Math::Vector3(48, 0, 0), Math::Vector3(136, 0, 0), 8.0f }
	}
},

	// ステージ4：A側は魔法の橋、B側は階段で高台へ。ブロック2個を両スイッチに置くと檻が開く
	{
		Math::Vector3(0, 0, 0), Math::Vector3(248, -2.5f, 0), 0.0f, 90.0f,
		true,
		{ Math::Vector3(200, 0, -24), Math::Vector3(200, 0, 24) },
		Math::Vector3(216, 0, 0),
		{ Math::Vector3(104, 4, -104), Math::Vector3(104, 52, 96) },	// Bは高台48の上
		"Asset/Data/MapData/Map4.csv", 4, 15,
		2,																// 通常の建築上限
		50.0f,															// cageHeight
		5, Math::Vector2(60, 44), Math::Vector2(148, 108),				// 南の部屋だけ5層
		Math::Vector3(73.0f, 57.0f, 45.0f)								// cageScale
	},

	// ステージ5：ハブ型。A・B・Cのスイッチを全部押すと、真ん中のゴールを囲う檻4つが開く
{
	Math::Vector3(0, 0, 0), Math::Vector3(0, -2.5f, -128), 0.0f, 180.0f,
	true,
	{ Math::Vector3(-135, 0, -127),		// A
	  Math::Vector3(304, 0, -128),		// B
	  Math::Vector3(48, 0, -256) },		// C
	Math::Vector3(24, 0, -128),			// cageBasePos(東)
	{ Math::Vector3(-288, 28, -128),	// A：棚の上
	  Math::Vector3(128, 4, -128),		// B：入口の足場
	  Math::Vector3(0, 52, -328) },		// C：塔の上
	"Asset/Data/MapData/Map5.csv", 39, 45,
	2,
	50.0f,
	5, Math::Vector2(-84, -348), Math::Vector2(84, -228),
	Math::Vector3(41.0f, 57.0f, 45.0f),
	{
		{ Math::Vector3(168, 0, -160), Math::Vector3(168, 0, -176), 5.0f },
		{ Math::Vector3(168, 0, -96),  Math::Vector3(168, 0, -80),  6.5f },
		{ Math::Vector3(196, 0, -128), Math::Vector3(220, 0, -128), 6.0f },
		{ Math::Vector3(248, 0, -176), Math::Vector3(248, 0, -144), 5.5f },
		{ Math::Vector3(248, 0, -80),  Math::Vector3(248, 0, -112), 7.0f },
	},
	{
		{ Math::Vector3(-24, 0, -128), 90.0f },	// 西
		{ Math::Vector3(0, 0, -152),   0.0f },	// 北
		{ Math::Vector3(0, 0, -104),   0.0f },	// 南
	}
},
};

static const int g_stageCount = sizeof(g_stageTable) / sizeof(g_stageTable[0]);