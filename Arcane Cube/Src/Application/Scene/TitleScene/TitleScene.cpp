#include "TitleScene.h"
#include "../SceneManager.h"
#include "TitleTexture/TitleTexture.h"
#include "../../GameObject/UI/Mouse/Mouse.h"

void TitleScene::Event()
{
	if (GetAsyncKeyState(VK_RETURN) & 0x8000 || Mouse::Instance().IsClick())
	{
		SceneManager::Instance().RequestChangeScene(SceneManager::SceneType::Game);
	}
}

void TitleScene::Init()
{
	Mouse::Instance().ShowMouse(true);

	auto titletexture = std::make_shared<TitleTexture>();
	titletexture->Init();
	AddObject(titletexture);
}
