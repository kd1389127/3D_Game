#include "GameScene.h"
#include"../SceneManager.h"

#include "../../GameObject/Camera/FPSCamera/FPSCamera.h"
#include "../../GameObject/Map/Ground/Ground.h"
#include "../../GameObject/Character/Player/Player.h"
#include "../../GameObject/Weapon/Magicwand/Magicwand.h"

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
	// Map(地面)
	std::shared_ptr<Ground> ground;
	ground = std::make_shared<Ground>();
	ground->Init();
	m_objList.push_back(ground);

	// Player
	std::shared_ptr<Player> player;
	player = std::make_shared<Player>();
	player->Init();
	m_objList.push_back(player);

	// 魔法の杖
	std::shared_ptr<Magicwand> magicwand;
	magicwand = std::make_shared<Magicwand>();
	magicwand->Init();
	m_objList.push_back(magicwand);

	// FPSカメラ
	std::shared_ptr<FPSCamera> fpscamera;
	fpscamera = std::make_shared<FPSCamera>();
	fpscamera->Init();
	m_objList.push_back(fpscamera);

	// 各オブジェクトに必要なデータを渡しておく
	fpscamera->SetTarget(player);	// カメラに注視対象(プレイヤー)をセット
	magicwand->SetParent(player);

}
