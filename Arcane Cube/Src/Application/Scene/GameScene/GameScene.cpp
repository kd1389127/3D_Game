#include "GameScene.h"
#include "../SceneManager.h"
#include "../StageData.h"
#include "../../GameObject/Block/BlockGridManager.h"
#include "../../GameObject/Magic/MagicManager.h"

#include "../../GameObject/Camera/FPSCamera/FPSCamera.h"
#include "../../GameObject/Map/Ground/Ground.h"
#include "../../GameObject/Character/Player/Player.h"
#include "../../GameObject/Weapon/Magicwand/Magicwand.h"
#include "../../GameObject/UI/Reticle/Reticle.h"
#include "../../GameObject/UI/MagicGaugeUI/MagicGaugeUI.h"
#include "../../GameObject/Block/GimmickBlock/GimmickBlock.h"
#include "../../GameObject/Map/Gimmick/Goal/Goal.h"
#include "../../GameObject/Map/Gimmick/Cage/Cage.h"
#include "../../GameObject/Map/Gimmick/Switch/Switch.h"
#include "../../GameObject/Map/MapBackGround/MapBackGround.h"
#include "../../GameObject/Map/MapLoader.h"
#include "../../GameObject/UI/Mouse/Mouse.h"

void GameScene::Event()
{
	if (GetAsyncKeyState('T') & 0x8000)
	{
		SceneManager::Instance().SetCurrentStage(0); // タイトルに戻る＝進行状況をリセット
		SceneManager::Instance().SetNextScene(SceneManager::SceneType::Title);
	}
	// Gキーでデバッググリッドの表示切り替え
	static bool prevG = false;
	bool nowG = (GetAsyncKeyState('G') & 0x8000) != 0;
	if (nowG && !prevG) // 押した瞬間だけ反応させる(トリガー判定)
	{
		BlockGridManager::Instance().ToggleDebugGrid();
	}
	prevG = nowG;

	// PageUp/PageDownでグリッドを表示する高さ(段)を切り替える
	static bool prevUp = false, prevDown = false;
	bool nowUp = (GetAsyncKeyState(VK_UP) & 0x8000) != 0;
	bool nowDown = (GetAsyncKeyState(VK_DOWN) & 0x8000) != 0;
	if (nowUp && !prevUp) { BlockGridManager::Instance().ChangeDebugLayer(1); }
	if (nowDown && !prevDown) { BlockGridManager::Instance().ChangeDebugLayer(-1); }
	prevUp = nowUp;
	prevDown = nowDown;

	// ステージを切り替え
	static bool prev1 = false, prev2 = false;
	bool now1 = (GetAsyncKeyState('1') & 0x8000) != 0;
	bool now2 = (GetAsyncKeyState('2') & 0x8000) != 0;

	if (now1 && !prev1)
	{
		SceneManager::Instance().SetCurrentStage(0);
		SceneManager::Instance().ReloadScene();
	}
	if (now2 && !prev2)
	{
		SceneManager::Instance().SetCurrentStage(1);
		SceneManager::Instance().ReloadScene();
	}
	prev1 = now1;
	prev2 = now2;
}

void GameScene::Init()
{
	// マウス関連
	Mouse::Instance().ShowMouse(false);
	Mouse::Instance().ResetCursorToCenter(); // 前回位置を中央にそろえる

	// ステージ開始時は魔力を満タンにリセット
	MagicManager::Instance().Reset();

	// 現在のステージ情報を取得
	int stage = SceneManager::Instance().GetCurrentStage();
	const StageData& data = g_stageTable[stage];

	// 前ステージの占有情報をクリアし、今ステージの地面の高さをグリッド基準にする
	// MapLoader::Loadはこの後に呼ぶこと(順番が重要)	
	BlockGridManager::Instance().Clear();
	BlockGridManager::Instance().SetGroundHeight(data.groundHeight);

	// ステージ開始時は必ずグリッド非表示から始める
	if (BlockGridManager::Instance().IsDebugGridVisible())
	{
		BlockGridManager::Instance().ToggleDebugGrid();
	}

	// Map背景
	std::shared_ptr<MapBackGround> mapbackground;
	mapbackground = std::make_shared<MapBackGround>();
	mapbackground->Init();
	mapbackground->SetBaseY(data.groundHeight - 100.0f);
	m_objList.push_back(mapbackground);

	// Map(地面)
	// Groundは「空」のまま置いておく(デバッググリッドの描画や、FindGroundでの判定に使うため)
	std::shared_ptr<Ground> ground;
	ground = std::make_shared<Ground>();
	m_objList.push_back(ground);

	// CSVからMapのブロックを生成する
	MapLoader loader;
	std::vector<std::shared_ptr<KdGameObject>> mapBlocks;
	loader.Load(data.mapCsvPath, data.groundHeight, data.csvOriginCol, data.csvOriginRow, mapBlocks);

	for (auto& block : mapBlocks)
	{
		m_objList.push_back(block);
	}

	// Player
	std::shared_ptr<Player> player;
	player = std::make_shared<Player>();
	player->Init(data.playerStartPos, data.groundHeight, data.playerStartYaw);
	m_objList.push_back(player);

	// 魔法の杖
	std::shared_ptr<Magicwand> magicwand;
	magicwand = std::make_shared<Magicwand>();
	magicwand->Init();
	m_objList.push_back(magicwand);

	// レティクル
	std::shared_ptr<Reticle> reticle;
	reticle = std::make_shared<Reticle>();
	reticle->Init();
	reticle->SetTarget(player); 
	m_objList.push_back(reticle);

	// 魔力ゲージ
	std::shared_ptr<MagicGaugeUI> magicgaugeUI;
	magicgaugeUI = std::make_shared<MagicGaugeUI>();
	magicgaugeUI->Init();
	m_objList.push_back(magicgaugeUI);

	// FPSカメラ
	std::shared_ptr<FPSCamera> fpscamera;
	fpscamera = std::make_shared<FPSCamera>();
	fpscamera->Init();
	m_objList.push_back(fpscamera);

	// ゴール
	std::shared_ptr<Goal> goal;
	goal = std::make_shared<Goal>();
	goal->Init(data.goalPos);
	m_objList.push_back(goal);

	// スイッチ・檻・ギミックブロックは、このステージで必要な場合のみ生成する
	if (data.hasGimmickCage)
	{
		// スイッチ
		std::shared_ptr<Switch> sw;
		sw = std::make_shared<Switch>();
		sw->Init(data.switchPos);
		m_objList.push_back(sw);

		// 檻(鉄格子)：スイッチと紐付けて生成
		std::shared_ptr<Cage> cage;
		cage = std::make_shared<Cage>();
		cage->Init(sw, data.cageBasePos, 32.0f, 0.2f);  //数値はmaxHeight,speed
		m_objList.push_back(cage);

		// ギミック専用ブロック(スイッチに置くための鍵ブロック)
		std::shared_ptr<GimmickBlock> gimmickBlock;
		gimmickBlock = std::make_shared<GimmickBlock>();
		gimmickBlock->Init(data.gimmickBlockPos);
		BlockGridManager::Instance().Register(
			BlockGridManager::Instance().SnapToGrid(data.gimmickBlockPos),
			BlockGridManager::BlockKind::GimmickKey);
		m_objList.push_back(gimmickBlock);
	}

	// 各オブジェクトに必要なデータを渡しておく
	fpscamera->SetTarget(player);	// カメラに注視対象(プレイヤー)をセット
	magicwand->SetParent(player);
	mapbackground->SetTarget(player);
	fpscamera->SetMagicwand(magicwand);

}