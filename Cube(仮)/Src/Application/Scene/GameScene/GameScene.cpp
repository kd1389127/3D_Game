#include "GameScene.h"
#include "../SceneManager.h"
#include "../StageData.h"
#include "../../GameObject/Block/BlockGridManager.h"

#include "../../GameObject/Camera/FPSCamera/FPSCamera.h"
#include "../../GameObject/Map/Ground/Ground.h"
#include "../../GameObject/Character/Player/Player.h"
#include "../../GameObject/Weapon/Magicwand/Magicwand.h"
#include "../../GameObject/UI/Reticle/Reticle.h"
#include "../../GameObject/Block/GimmickBlock/GimmickBlock.h"
#include "../../GameObject/Map/Gimmick/Goal/Goal.h"
#include "../../GameObject/Map/Gimmick/Cage/Cage.h"
#include "../../GameObject/Map/Gimmick/Switch/Switch.h"

void GameScene::Event()
{
	if (GetAsyncKeyState('T') & 0x8000)
	{
		SceneManager::Instance().SetCurrentStage(0); // タイトルに戻る＝進行状況をリセット
		SceneManager::Instance().SetNextScene(SceneManager::SceneType::Title);
	}
}

void GameScene::Init()
{
	// 前のステージで置かれたブロックの占有情報をクリア
	BlockGridManager::Instance().Clear();

	// 現在のステージ情報を取得
	int stage = SceneManager::Instance().GetCurrentStage();
	const StageData& data = g_stageTable[stage];

	// ステージ切り替え時：前ステージの占有情報をクリアし、今ステージの地面の高さをグリッド基準にする
	BlockGridManager::Instance().Clear();
	BlockGridManager::Instance().SetGroundHeight(data.groundHeight);
	
	// Map(地面)
	std::shared_ptr<Ground> ground;
	ground = std::make_shared<Ground>();
	ground->Init(data.mapModelPath, data.mapScale);
	m_objList.push_back(ground);

	// Player
	std::shared_ptr<Player> player;
	player = std::make_shared<Player>();
	player->Init(data.playerStartPos, data.groundHeight);
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
	m_objList.push_back(reticle);

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

}
