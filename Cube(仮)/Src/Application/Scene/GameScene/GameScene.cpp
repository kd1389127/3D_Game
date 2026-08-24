#include "GameScene.h"
#include "../SceneManager.h"
#include "../StageData.h"

#include "../../GameObject/Camera/FPSCamera/FPSCamera.h"
#include "../../GameObject/Map/Ground/Ground.h"
#include "../../GameObject/Character/Player/Player.h"
#include "../../GameObject/Weapon/Magicwand/Magicwand.h"
#include "../../GameObject/UI/Reticle/Reticle.h"
#include "../../GameObject/Map/Gimmick/Goal/Goal.h"

void GameScene::Event()
{
	if (GetAsyncKeyState('T') & 0x8000)
	{
		SceneManager::Instance().SetNextScene
		(
			SceneManager::SceneType::Title
		);
	}
}

void GameScene::Init()
{
	// 現在のステージ情報を取得
	int stage = SceneManager::Instance().GetCurrentStage();
	const StageData& data = g_stageTable[stage];

	// Map(地面)
	std::shared_ptr<Ground> ground;
	ground = std::make_shared<Ground>();
	ground->Init();
	m_objList.push_back(ground);

	// Player
	std::shared_ptr<Player> player;
	player = std::make_shared<Player>();
	player->Init(data.playerStartPos);
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

	// 各オブジェクトに必要なデータを渡しておく
	fpscamera->SetTarget(player);	// カメラに注視対象(プレイヤー)をセット
	magicwand->SetParent(player);

}
