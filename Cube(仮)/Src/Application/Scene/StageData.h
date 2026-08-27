#pragma once

// 1ステージ分の配置情報
struct StageData
{
	Math::Vector3 playerStartPos;		 // プレイヤーの初期座標
	Math::Vector3 goalPos;				 // ゴールの座標
	std::string   mapModelPath;			 // Mapのモデル
	float         mapScale;				 // Mapの大きさ指定
	float         groundHeight = 0.0f;   // ブロックの下限に使う、そのステージの地面のY GetMinY()が0.0f + 4.0f = 4.0f
	
	// スイッチ・檻(鉄格子)・ギミック専用ブロックの配置(このステージで使う場合のみ)
	bool          hasGimmickCage = false;					// このステージにスイッチ/檻/ギミックブロックを配置するか
	Math::Vector3 switchPos = Math::Vector3::Zero;			// スイッチの座標
	Math::Vector3 cageBasePos = Math::Vector3::Zero;		// 檻(鉄格子)の基準座標(閉じている時の位置)
	Math::Vector3 gimmickBlockPos = Math::Vector3::Zero;	// ギミック専用ブロックの初期配置座標
};

// ステージテーブル(ステージを増やす場合はここに追加していくだけでOK)
static const StageData g_stageTable[] =
{
	// ステージ0
	// プレイヤーのスタート位置	   ゴールの位置		Model	Modelサイズ		地面の高さ指定
	{ Math::Vector3(50, 0, 0), Math::Vector3(-125, 30, 0), "Asset/Models/Map/Map1/Map1.gltf" ,	30.0f,	0.0f},
	
	// ステージ1(スイッチ・檻・ギミックブロックの座標)
	{ Math::Vector3(50, 15, 0), Math::Vector3(-65, 45, 0), "Asset/Models/Map/Map2/Map2.gltf", 20.0f, 13.0f,
	  true,								// hasGimmickCage
	  Math::Vector3(0, 13.0f, -16),		// switchPos
	  Math::Vector3(-45, 35.0f, -0.5),  // cageBasePos
	  Math::Vector3(25, 65.0f, 26.5)	// gimmickBlockPos
	},

	//// ステージ2(仮の値。後で調整してください)
	//{ Math::Vector3(0, 0, 0),      Math::Vector3(0, 0, 0) },
};

static const int g_stageCount = sizeof(g_stageTable) / sizeof(g_stageTable[0]);