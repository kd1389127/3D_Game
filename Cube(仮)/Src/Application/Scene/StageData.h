#pragma once

// 1ステージ分の配置情報
struct StageData
{
	Math::Vector3 playerStartPos;   // プレイヤーの初期座標
	Math::Vector3 goalPos;          // ゴールの座標
};

// ステージテーブル(ステージを増やす場合はここに追加していくだけでOK)
static const StageData g_stageTable[] =
{
	// ステージ0
	// プレイヤーのスタート位置	   ゴールの位置
	{ Math::Vector3(50, 0, 0),     Math::Vector3(-125, 30, 0) },

	// ステージ1(仮の値。後で調整してください)
	//{ Math::Vector3(0, 0, 0),      Math::Vector3(0, 0, 0) },

	//// ステージ2(仮の値。後で調整してください)
	//{ Math::Vector3(0, 0, 0),      Math::Vector3(0, 0, 0) },
};

static const int g_stageCount = sizeof(g_stageTable) / sizeof(g_stageTable[0]);