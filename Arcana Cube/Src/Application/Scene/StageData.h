#pragma once

// オブジェクティブ(画面左の「今やること」)の種類。
enum class ObjectiveKind : int
{
	Aim,		// 右クリック長押しで狙う
	Wheel,		// ホイールで長さを決める
	Shoot,		// 左クリックで撃ってブロックを作る
	Goal,		// ゴールへ向かう
	Grab,		// ブロックを掴み置く
	OneSwitch,	// スイッチを起動する
	LiftSwitch,	// リフトに乗ってスイッチを起動する
	Switches,	// すべてのスイッチを起動させる
	Look,		// マウスで見回す
	Move,		// WASDで移動する
	Jump,		// Spaceでジャンプする
	GetWand,	// 台座の杖を取る
};

// Hキーで出すヒントの種類
enum class HintKind : int
{
	None = -1, // ヒントなし
	Generate,  // 生成・魔力・削除
	Carry,	   // 運ぶ・置く・スイッチ・檻
	Wall,	   // 生成できない壁
	Lift,	   // リフト
	Count,	   // 種類の数
};


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

// 魔法ブロックを置けない箱(ワールド座標)。奈落の上など、抜け道をふさぐのに使う
struct ForbiddenZone
{
	Math::Vector3 min = Math::Vector3::Zero;
	Math::Vector3 max = Math::Vector3::Zero;
};

// 「この部屋の中にいる間は、落ちたらここに戻る」という中間地点(部屋ごとに置く)
struct CheckpointZone
{
	Math::Vector3 min = Math::Vector3::Zero;		// 箱の最小(ワールド座標)
	Math::Vector3 max = Math::Vector3::Zero;		// 箱の最大
	Math::Vector3 respawnPos = Math::Vector3::Zero;	// 落ちた時に戻る場所
	float yaw = 0.0f;								// 戻った時の向き(度、0=+Z)
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
	int           csvOriginCol = 0;									// CSV上でワールド原点(X=0)にあたる列
	int           csvOriginRow = 0;									// CSV上でワールド原点(Z=0)にあたる行
	int			  buildLimitLayers = 0;

	float         cageHeight = 32.0f;								// 檻が開く高さ

	int           tallBuildLimitLayers = 0;							// 高く積めるエリアの上限層数(0ならなし)
	Math::Vector2 tallZoneMin = Math::Vector2::Zero;				// エリアのXZ最小(yにはワールドZが入る)
	Math::Vector2 tallZoneMax = Math::Vector2::Zero;				// エリアのXZ最大

	Math::Vector3 cageScale = Math::Vector3(55.0f, 45.0f, 45.0f);	// 檻の大きさ
	std::vector<LiftData> lifts;									// リフト(なければ空)
	std::vector<CageData> extraCages;
	std::vector<ForbiddenZone> forbiddenZones;						// 魔法ブロックを置けない場所(なければ空)
	std::vector<CheckpointZone> checkpoints;						// 部屋ごとの中間地点(なければ空)

	std::vector<ObjectiveKind> objectives = { ObjectiveKind::Goal };// 出す項目(上から順)。空なら出さない
	Math::Vector3 wandPickupPos = Math::Vector3::Zero;				// 台座の杖の位置(チュートリアルだけ。浮かせる高さ)
	bool          buildOnlyNextToMap = false;						// trueなら、魔法ブロックはMapの面(1)に接したマスにしか置けない

	HintKind	hint = HintKind::None;								// Hキーで出すヒント
};

// ステージ番号(g_stageTableの添字)。番号を直書きせず、これを使う
static constexpr int kStageTutorial = 0;
static constexpr int kStageMap1		= 1;	
static constexpr int kStageMap2		= 2;

// ステージテーブル(ステージを増やす場合はここに追加していくだけでOK)
static const StageData g_stageTable[] =
{
	// ステージ1：段差を登り、穴の飛び石を渡って、杖を取ってゴールへ
	{
		Math::Vector3(0, 16, 0), Math::Vector3(-235, 13.5, 0), 0.0f, -90.0f,	// 1つ目=スタート、2つ目=ゴール
		false, {}, Math::Vector3::Zero, {},
		"Asset/Data/MapData/Stage1.csv", 32, 4,		// originCol=32, originRow=4
		0,
		32.0f,
		0, Math::Vector2::Zero, Math::Vector2::Zero,
		Math::Vector3(55.0f, 45.0f, 45.0f),
		{}, {},
		{
			// 全体を魔法ブロック禁止
			{ Math::Vector3(-1000, -1000, -1000), Math::Vector3(1000, 1000, 1000) }
		},
		{},
		{ ObjectiveKind::Look, ObjectiveKind::Move, ObjectiveKind::Jump, ObjectiveKind::GetWand, ObjectiveKind::Goal },
		Math::Vector3(-200.0f, 24.0f, 0.0f),						// wandPickupPos(台座の上)
	},

	// ステージ2
	// プレイヤー開始位置、ゴールの位置、ステージの下限、プレイヤーの向き
	// ギミックを使うか、ギミックの各配置、CSVデータ、CSVの列、CSVの行、置ける最大の上面
	{
		Math::Vector3(80, 0, 10), Math::Vector3(-100, 13.5, 8.0f), 0.0f,-90.0f,
		false, {}, Math::Vector3::Zero,{},
		"Asset/Data/MapData/Stage2.csv", 14, 4,
		0,
		32.0f,
		0,Math::Vector2::Zero,Math::Vector2::Zero,
		Math::Vector3(55.0f,45.0f,45.0f),
		{},{},{},{},
		{ObjectiveKind::Aim,ObjectiveKind::Wheel,ObjectiveKind::Shoot,ObjectiveKind::Goal},
		Math::Vector3::Zero,
		false,
		HintKind::Generate,
	},

	// ステージ3(スイッチ・檻・ギミックブロックの座標)
	{
		Math::Vector3(48, 0, 0), Math::Vector3(-60, 21.5, 0), 0.0f,-90.0f,
		true,
		{Math::Vector3(0, 0, -16)},			// switchPositions
		Math::Vector3(-48, 24.0f, 0),		// cageBasePos
		{Math::Vector3(64, 52.0f, 0)},		// gimmickBlockPositions
		"Asset/Data/MapData/Stage3.csv", 14, 5,
		7,
		32.0f,
		0, Math::Vector2::Zero, Math::Vector2::Zero,
		Math::Vector3(55.0f, 45.0f, 45.0f),
		{}, {}, {}, {},
		{  ObjectiveKind::Grab,ObjectiveKind::OneSwitch, ObjectiveKind::Goal },
		Math::Vector3::Zero,
		false,
		HintKind::Carry,
	},
	
	// ステージ4：魔力と削除。積んで登る→橋で奈落を渡る→削除で魔力を回収して高台へ
	{
		Math::Vector3(-48, 0, 0), Math::Vector3(153, 69.5f, 0), 0.0f, 90.0f,	// スタートを西へ48ずらす
		false, {}, Math::Vector3::Zero, {},
		"Asset/Data/MapData/Stage4.csv", 8, 3,	// originCol=8(2+6列)、originRow=3
		9,										// buildLimitLayers
		32.0f,
		0, Math::Vector2::Zero, Math::Vector2::Zero,
		Math::Vector3(55.0f, 45.0f, 45.0f),
		{}, {}, {},
		{
			// 奈落から落ちたら高台1の上に戻る
			{
				Math::Vector3(28, -1000, -28), Math::Vector3(108, 1000, 28),	// 高台1〜高台2の範囲
				Math::Vector3(40, 24, 0), 90.0f									// 戻る位置、向き
			},
		},
		{ ObjectiveKind::Goal },
	},

	// ステージ5：魔力を吸う壁。暗い壁の中の、生成できる場所を渡って登る
	{
		Math::Vector3(0, 0, 0), Math::Vector3(240, 53.5f, 0), 0.0f, 90.0f,
		false, {}, Math::Vector3::Zero, {},
		"Asset/Data/MapData/Stage5.csv", 2, 5,	// originCol=2, originRow=5(端に余白の行を足したため)
		7,										// buildLimitLayers
		32.0f,
		0, Math::Vector2::Zero, Math::Vector2::Zero,
		Math::Vector3(55.0f, 45.0f, 45.0f),
		{}, {},
		{},										// forbiddenZones：なし(CSVの6と1で決める)
		{},
		{ ObjectiveKind::Goal },
		Math::Vector3::Zero,					// wandPickupPos(使わない)
		true,									// buildOnlyNextToMap
		HintKind::Wall,
	},

	// ステージ6：リフトの紹介。ブロックを持ってリフトで奈落を渡り、スイッチに置いて檻を開ける
	{
		Math::Vector3(0, 0, 0), Math::Vector3(208, -2.5f, 0), 0.0f, 90.0f,
		true,
		{ Math::Vector3(176, 0, 0) },					// switchPositions
		Math::Vector3(192, 0, 0),						// cageBasePos
		{ Math::Vector3(18, 4, 16) },					// gimmickBlockPositions
		"Asset/Data/MapData/Stage6.csv", 2, 4,			// originCol=2, originRow=4
		4,												// buildLimitLayers
		32.0f,											// cageHeight
		0, Math::Vector2::Zero, Math::Vector2::Zero,	// 高く積めるエリアなし
		Math::Vector3(55.0f, 40.0f, 45.0f),				// cageScale
		{
		// 変更後：始点をゴール側(140)、終点をプレイヤー側(44)にする
		{ Math::Vector3(140, 0, 0), Math::Vector3(44, 0, 0), 5.0f }
		},
		{},
		{
			// 奈落の上は、高さも奥行きも全部、魔法ブロックを置けない(リフトを使わせる)
			{ Math::Vector3(36, -1000, -1000), Math::Vector3(148, 1000, 1000) }
		},
		{},
		{ObjectiveKind::LiftSwitch,ObjectiveKind::Goal},
		Math::Vector3::Zero,
		false,
		HintKind::Lift,
	},

	// ステージ7：魔力のやりくり。高台の上の2つのスイッチを押して、檻を開ける
	{
		Math::Vector3(0, 24, 0), Math::Vector3(192, -2.5f, 0), 0.0f, 90.0f,	// スタートは高台の上
		true,
		{ Math::Vector3(96, 24, -48), Math::Vector3(96, 24, 48) },			// 北と南の高台の上
		Math::Vector3(160, 0, 0),											// 檻(仕切りの出入口)
		{ Math::Vector3(40, 4, -32), Math::Vector3(40, 4, 32) },			// ブロック2つ(スタートから見える床)
		"Asset/Data/MapData/Stage7.csv", 2, 8,
		3,																	// buildLimitLayers(高台と同じ高さまで)
		24.0f,																// cageHeight
		0, Math::Vector2::Zero, Math::Vector2::Zero,
		Math::Vector3(40.0f, 40.0f, 36.0f),									// cageScale(壁にめり込まないよう細く)
		{}, {}, {}, {},
		{ ObjectiveKind::Switches, ObjectiveKind::Goal },
	},

	// ステージ8：橋とリフトで、ブロックを落とさずに運ぶ
	{
		Math::Vector3(0, 0, 0), Math::Vector3(264, -2.5f, 0), 0.0f, 90.0f,
		true,
		{ Math::Vector3(224, 0, -24) },									// 着地場所のスイッチ
		Math::Vector3(240, 0, 0),										// 檻(仕切りの出入口)
		{ Math::Vector3(24, 4, 0) },									// ブロック(スタートの目の前)
		"Asset/Data/MapData/Stage8.csv", 2, 5,
		3,																// buildLimitLayers
		24.0f,															// cageHeight
		0, Math::Vector2::Zero, Math::Vector2::Zero,
		Math::Vector3(40.0f, 40.0f, 36.0f),								// cageScale(新Cと同じ)
		{
			// 始点を着地場所側、終点を島側にする(Map3と同じ向き)。奈落2の両端から8だけ内側
			{ Math::Vector3(196, 0, 0), Math::Vector3(124, 0, 0), 5.0f }
		},
		{},
		{
			// 奈落2の上は、高さも奥行きも全部、魔法ブロックを置けない(リフトを使わせる)
			// 奈落2のマス中心はx=120〜200なので、その外側のマス(112と208)を含まない範囲にする
			{ Math::Vector3(116, -1000, -1000), Math::Vector3(204, 1000, 1000) }
		},
		{
			// 奈落2で落ちたら、島に戻る
			{
				Math::Vector3(84, -1000, -48), Math::Vector3(204, 1000, 48),	// 島〜奈落2の範囲
				Math::Vector3(96, 0, 0), 90.0f									// 戻る位置、向き
			}
		},
		{ ObjectiveKind::LiftSwitch, ObjectiveKind::Goal },
	},

	// ステージ9：A側は魔法の橋、B側は階段で高台へ。ブロック2個を両スイッチに置くと檻が開く
	{
		Math::Vector3(0, 0, 0), Math::Vector3(248, -2.5f, 0), 0.0f, 90.0f,
		true,
		{ Math::Vector3(200, 0, -24), Math::Vector3(200, 0, 24) },
		Math::Vector3(216, 0, 0),
		{ Math::Vector3(104, 4, -104), Math::Vector3(104, 52, 96) },	// Bは高台48の上
		"Asset/Data/MapData/Stage9.csv", 4, 15,
		5,																// 通常の建築上限
		50.0f,															// cageHeight
		0, Math::Vector2::Zero, Math::Vector2::Zero,
		Math::Vector3(73.0f, 57.0f, 45.0f),								// cageScale
		{}, {}, {}, {},
		{ObjectiveKind::Switches,ObjectiveKind::Goal}
	},

	// ステージ10：ハブ型。A・B・Cのスイッチを全部押すと、真ん中のゴールを囲う檻4つが開く
	{
		Math::Vector3(0, 0, 0), Math::Vector3(0, -2.5f, -128), 0.0f, 180.0f,
		true,
		{ Math::Vector3(-135, 0, -127),		// A
		  Math::Vector3(128, 0, -128),		// B
		  Math::Vector3(0, 0, -256) },		// C
		Math::Vector3(24, 0, -128),			// cageBasePos(東)
		{ Math::Vector3(-288, 28, -128),	// A：棚の上
		  Math::Vector3(304, 4, -128),		// B
		  Math::Vector3(0, 52, -328) },		// C：塔の上
		"Asset/Data/MapData/Stage10.csv", 39, 45,
		5,
		50.0f,
		0, Math::Vector2::Zero, Math::Vector2::Zero,
		Math::Vector3(41.0f, 57.0f, 45.0f),
		{
			{ Math::Vector3(168, 0, -176), Math::Vector3(168, 0, -144), 3.5f },
			{ Math::Vector3(168, 0, -80),  Math::Vector3(168, 0, -112), 3.5f },
			{ Math::Vector3(220, 0, -128), Math::Vector3(196, 0, -128), 3.5f },
			{ Math::Vector3(248, 0, -144), Math::Vector3(248, 0, -176), 3.5f },
			{ Math::Vector3(248, 0, -112), Math::Vector3(248, 0, -80),  3.5f },
		},

		{
			{ Math::Vector3(-24, 0, -128), 90.0f },	// 西
			{ Math::Vector3(0, 0, -152),   0.0f },	// 北
			{ Math::Vector3(0, 0, -104),   0.0f },	// 南
		},

		{
			// リフト部屋の奈落の上。高さも全部、魔法ブロックを置けない(リフトを使わせる)
			// 穴のマス中心はx=152〜264なので、その外側のマス(144と272)を含まない範囲にする
			{ Math::Vector3(148, -1000, -196), Math::Vector3(268, 1000, -60) }
		},
		
		{
			// ---- 奈落のある部屋ごとの中間地点(落ちたら、その部屋の入口に戻る) ----
			// 箱の範囲は仮。部屋の端でPキーを押して、座標を見て広げる
			{
				// A部屋(西)：入口の足元の座標
				Math::Vector3(-330, -1000, -196), Math::Vector3(-100, 1000, -60),	// 箱(最小・最大)
				Math::Vector3(-110, 0, -128), -90.0f								// 戻る位置、向き
			},
			{
				// リフト部屋(東)：箱の最小Xは、戻る位置(105)より少し手前の100にする
				Math::Vector3(100, -1000, -196), Math::Vector3(330, 1000, -56),
				Math::Vector3(105, 0, -128), 90.0f
			}
		},
		{ObjectiveKind::Switches,ObjectiveKind::Goal}
	},

	// GoalMap：階段を上ると画面が白くなって、リザルトへ(ゴールは置かない。常に最後のステージにする)
	{
		Math::Vector3(16, 0, 0), Math::Vector3::Zero, 0.0f, -90.0f,
		false, {}, Math::Vector3::Zero, {},
		"Asset/Data/MapData/GoalMap.csv", 8, 2,
		0,												// buildLimitLayers(下で全部禁止するので使わない)
		32.0f,											// cageHeight(檻がないので使わない)
		0, Math::Vector2::Zero, Math::Vector2::Zero,	// 高く積めるエリアなし
		Math::Vector3(55.0f, 45.0f, 45.0f),				// cageScale(使わない)
		{},												// lifts
		{},												// extraCages
		{
			// 全体を、魔法ブロック禁止にする(階段を飛ばしたり、壁を越えたりさせない)
			{ Math::Vector3(-1000, -1000, -1000), Math::Vector3(1000, 1000, 1000) }
		}
	},

};

static const int g_stageCount = sizeof(g_stageTable) / sizeof(g_stageTable[0]);