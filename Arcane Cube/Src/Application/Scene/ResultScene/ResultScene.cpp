#include "ResultScene.h"
#include "../SceneManager.h"
#include "../../GameObject/UI/Mouse/Mouse.h"

void ResultScene::Event()
{
	if (GetAsyncKeyState(VK_RETURN) & 0x8000 || Mouse::Instance().IsClick())
	{
		SceneManager::Instance().RequestChangeScene(SceneManager::SceneType::Title, 0);
	}
}

void ResultScene::Init()
{

}
