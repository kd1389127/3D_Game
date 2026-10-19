#include "ResultScene.h"
#include "../SceneManager.h"
#include "../../GameObject/UI/Mouse/Mouse.h"
#include "ResultTexture/ResultTexture.h"

static int  s_frame = 0;		// リザルトに入ってからの経過フレーム
static bool s_clicked = false;	// クリック音をもう鳴らしたか

void ResultScene::Event()
{
	// クリア直前まで押していたキーで、すぐ飛ばされないための待ち時間(90フレーム=約1.5秒)
	// ※ResultTextureの案内が現れる時間(90)と同じ値にしておく
	s_frame++;
	if (s_frame < 90) { return; }

	// 何かのキー(マウスボタン含む)が押されているか
	bool pressed = false;
	for (int vk = 1; vk < 255; ++vk)
	{
		if (GetAsyncKeyState(vk) & 0x8000) { pressed = true; break; }
	}
	if (!pressed) { return; }

	if (!s_clicked)
	{
		s_clicked = true;
		auto se = KdAudioManager::Instance().Play("Asset/Sounds/SE/UI/Click.wav", false);
		if (se) { se->SetVolume(0.15f); }
	}
	SceneManager::Instance().RequestChangeScene(SceneManager::SceneType::Title, 0);
}

void ResultScene::Init()
{
	s_frame = 0;
	s_clicked = false;

	// BGM：前のシーン(ゲーム)の音を止めてから、リザルトの曲を鳴らす
	KdAudioManager::Instance().StopAllSound();
	auto bgm = KdAudioManager::Instance().Play("Asset/Sounds/BGM/Result/ResultBGM.wav", true);
	if (bgm) { bgm->SetVolume(0.05f); }

	auto resulttexture = std::make_shared<ResultTexture>();
	resulttexture->Init();
	AddObject(resulttexture);
}
