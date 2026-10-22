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
#include "../../GameObject/UI/Mouse/Mouse.h"
#include "../../GameObject/UI/PauseMenu/PauseMenu.h"
#include "../../GameObject/UI/Guide/Guide.h"
#include "../../GameObject/Block/GimmickBlock/GimmickBlock.h"
#include "../../GameObject/Map/Gimmick/Goal/Goal.h"
#include "../../GameObject/Map/Gimmick/Cage/Cage.h"
#include "../../GameObject/Map/Gimmick/Switch/Switch.h"
#include "../../GameObject/Map/Gimmick/Lift/Lift.h"
#include "../../GameObject/Map/MapBackGround/MapBackGround.h"
#include "../../GameObject/Map/MapLoader.h"
#include "../../GameObject/Map/ExitLight/ExitLight.h"

void GameScene::Event()
{
	// ======================================================================================
	// デバックキー
	// ======================================================================================
	if (GetAsyncKeyState('R') & 0x8000)
	{
		SceneManager::Instance().RequestChangeScene(SceneManager::SceneType::Result, 0);
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

	// 0キーで全ステージを解放する(ステージ選択を見せる用。今のステージからは動かない)
	static bool prev0 = false;
	const bool now0 = (GetAsyncKeyState('0') & 0x8000) != 0;
	if (now0 && !prev0)
	{
		SceneManager::Instance().UnlockAllStages(static_cast<int>(std::size(g_stageTable)));
	}
	prev0 = now0;

	static bool prev6 = false;
	bool now6 = (GetAsyncKeyState('6') & 0x8000) != 0;
	if (now6 && !prev6)
	{
		SceneManager::Instance().SetCurrentStage(g_stageCount - 1);
		SceneManager::Instance().ReloadScene();
	}
	prev6 = now6;
}

void GameScene::Init()
{
	// BGM：すでに鳴っていれば何もしない(ステージ移動のReloadSceneで鳴り直さないため)
	static std::shared_ptr<KdSoundInstance> s_bgm;
	if (!s_bgm || !s_bgm->IsPlaying())
	{
		KdAudioManager::Instance().StopAllSound();
		s_bgm = KdAudioManager::Instance().Play("Asset/Sounds/BGM/Game/GameBGM.wav", true);
		if (s_bgm) { s_bgm->SetVolume(0.05f); }
	}

	// マウス関連
	Mouse::Instance().ShowMouse(false);
	Mouse::Instance().ResetCursorToCenter(); // 前回位置を中央にそろえる

	// パーティクル関連
	KdEffekseerManager::GetInstance().Reset();

	// ステージ開始時は魔力を満タンにリセット
	MagicManager::Instance().Reset();

	// 現在のステージ情報を取得
	int stage = SceneManager::Instance().GetCurrentStage();
	const StageData& data = g_stageTable[stage];

	// 前ステージの占有情報をクリアし、今ステージの地面の高さをグリッド基準にする
	// MapLoader::Loadはこの後に呼ぶこと(順番が重要)	
	BlockGridManager::Instance().Clear();
	BlockGridManager::Instance().SetGroundHeight(data.groundHeight);
	BlockGridManager::Instance().SetBuildLimitLayers(data.buildLimitLayers);
	BlockGridManager::Instance().SetTallBuildZone(
		data.tallBuildLimitLayers, data.tallZoneMin, data.tallZoneMax);
	for (const auto& zone : data.forbiddenZones)
	{
		BlockGridManager::Instance().AddForbiddenZone(zone.min, zone.max);
	}

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
	player->Init(data.playerStartPos, data.groundHeight, data.playerStartYaw, &data.checkpoints);
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
	reticle->SetShowHint(stage == 1);	// Map2だけ(stageは0始まり)
	m_objList.push_back(reticle);

	// 操作説明(バナーはMap1だけ、Tabアイコンは全ステージ)
	std::shared_ptr<Guide> guide;
	guide = std::make_shared<Guide>();
	guide->Init(stage == 0);
	guide->SetTarget(player);
	m_objList.push_back(guide);

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

	// 最後のステージ(GoalMap)は、ゴールを置かない(階段を上ると画面が白くなって、リザルトへ移る)
	const bool isExitStage = (stage == g_stageCount - 1);

	if (isExitStage)
	{
		// ゴールマップのライト(階段を上ると白くなり、リザルトへ移る)
		std::shared_ptr<ExitLight> exitLight;
		exitLight = std::make_shared<ExitLight>();

		// originCol=8, originRow=2 の場合の位置(壁の手前面に0.3だけ出す)
		exitLight->Init(Math::Vector3(-59.7f, data.groundHeight + 64.0f, 0.0f));
		exitLight->SetTarget(player);		// 階段をどこまで上ったかを調べる相手
		m_objList.push_back(exitLight);
	}
	else
	{
		// ゴール
		std::shared_ptr<Goal> goal;
		goal = std::make_shared<Goal>();
		goal->Init(data.goalPos);
		// ゴールの周り(3×3マス)には魔法ブロックを置けない(ゴールを自分で塞ぐのを防ぐ)
		constexpr float kGoalZoneHalf = BlockGridManager::GridSize * 1.5f;  // 隣のマスまで含める
		constexpr float kGoalZoneHeight = BlockGridManager::GridSize * 3.0f;  // 頭の上の3マスまで
		constexpr float kGoalZoneBottom = BlockGridManager::GridSize * 0.5f;  // ゴールの高さのマスも含める
		BlockGridManager::Instance().AddForbiddenZone(
			Math::Vector3(data.goalPos.x - kGoalZoneHalf, data.goalPos.y - kGoalZoneBottom, data.goalPos.z - kGoalZoneHalf),
			Math::Vector3(data.goalPos.x + kGoalZoneHalf, data.goalPos.y + kGoalZoneHeight, data.goalPos.z + kGoalZoneHalf));
		goal->SetTarget(player);
		m_objList.push_back(goal);
	}


	// リフト(このステージに設定があれば生成する。なければ何もしない)
	for (const LiftData& liftData : data.lifts)
	{
		std::shared_ptr<Lift> lift;
		lift = std::make_shared<Lift>();
		lift->Init(liftData.startPos, liftData.endPos, liftData.seconds);
		m_objList.push_back(lift);
	}

	// スイッチ・檻・ギミックブロックは、このステージで必要な場合のみ生成する
	if (data.hasGimmickCage)
	{
		// スイッチ(座標の数だけ生成)
		std::vector<std::shared_ptr<Switch>> switches;
		for (const auto& pos : data.switchPositions)
		{
			std::shared_ptr<Switch> sw;
			sw = std::make_shared<Switch>();
			sw->Init(pos);
			m_objList.push_back(sw);
			switches.push_back(sw);
		}

		// 檻(鉄格子)：スイッチと紐付けて生成
		std::shared_ptr<Cage> cage;
		cage = std::make_shared<Cage>();
		cage->Init(switches, data.cageBasePos, data.cageScale, data.cageHeight, 0.2f);  // 高さはStageDataから
		m_objList.push_back(cage);

		// 追加の檻(最初の檻と同じスイッチで一緒に開く。ゴールを4面とも囲う時に使う)
		for (const CageData& cageData : data.extraCages)
		{
			std::shared_ptr<Cage> extraCage;
			extraCage = std::make_shared<Cage>();
			extraCage->Init(switches, cageData.pos, data.cageScale, data.cageHeight, 0.2f, cageData.yaw);
			m_objList.push_back(extraCage);
		}

		// ギミック専用ブロック(スイッチに置くための鍵ブロック)
		for (const auto& pos : data.gimmickBlockPositions)
		{
			std::shared_ptr<GimmickBlock> gimmickBlock;
			gimmickBlock = std::make_shared<GimmickBlock>();
			gimmickBlock->Init(pos);
			BlockGridManager::Instance().RegisterFreeBlock(gimmickBlock);
			m_objList.push_back(gimmickBlock);
		}
	}

	// 各オブジェクトに必要なデータを渡しておく
	fpscamera->SetTarget(player);	// カメラに注視対象(プレイヤー)をセット
	magicwand->SetParent(player);
	mapbackground->SetTarget(player);
	fpscamera->SetMagicwand(magicwand);
	KdEffekseerManager::GetInstance().SetCamera(fpscamera->WorkCamera());
	
	// Tabメニュー：一番手前に描くため、リストの最後に追加する
	m_pauseMenu = std::make_shared<PauseMenu>();
	m_pauseMenu->Init();
	m_objList.push_back(m_pauseMenu);
	m_pauseObj = m_pauseMenu;	// ポーズ中でも、これだけは更新を止めない
}

bool GameScene::IsPaused() const
{
	return m_pauseMenu && m_pauseMenu->IsOpen();
}