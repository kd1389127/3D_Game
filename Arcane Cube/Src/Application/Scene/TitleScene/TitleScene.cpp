#include "TitleScene.h"
#include "../SceneManager.h"
#include "TitleTexture/TitleTexture.h"
#include "../../GameObject/UI/Mouse/Mouse.h"

static bool s_clicked = false;	// クリック音をもう鳴らしたか

void TitleScene::Event()
{
	if (GetAsyncKeyState(VK_RETURN) & 0x8000 || Mouse::Instance().IsClick())
	//if (GetAsyncKeyState(VK_RETURN) & 0x8000)
	{
		if (!s_clicked)
		{
			s_clicked = true;
			auto se = KdAudioManager::Instance().Play("Asset/Sounds/SE/UI/Click.wav", false);
			if (se) { se->SetVolume(0.15f); }
		}
		SceneManager::Instance().RequestChangeScene(SceneManager::SceneType::Game);
	}
}

void TitleScene::Init()
{
	s_clicked = false;	// タイトルに戻ってきた時にも、また鳴るように

	KdAudioManager::Instance().StopAllSound();
	auto bgm = KdAudioManager::Instance().Play("Asset/Sounds/BGM/Title/TitleBGM.wav", true);
	if (bgm) { bgm->SetVolume(0.05f); }

	Mouse::Instance().ShowMouse(true);

	auto titletexture = std::make_shared<TitleTexture>();
	titletexture->Init();
	AddObject(titletexture);
}
