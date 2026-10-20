#include "ResultScene.h"
#include "../SceneManager.h"
#include "../../GameObject/UI/Mouse/Mouse.h"
#include "ResultTexture/ResultTexture.h"

static int  s_frame = 0;		// リザルトに入ってからの経過フレーム
static bool s_clicked = false;	// クリック音をもう鳴らしたか
static bool s_prevDown[256] = {};	// 前フレームに押されていたか

void ResultScene::Event()
{
	s_frame++;

	// 「今フレームで新しく押されたキー」があるかを調べる
	// ※待ち時間中も s_prevDown を更新し続けるのが大事(押しっぱなしを覚えるため)
	bool pressed = false;
	for (int vk = 1; vk < 255; ++vk)
	{
		const bool down = (GetAsyncKeyState(vk) & 0x8000) != 0;
		if (down && !s_prevDown[vk]) { pressed = true; }
		s_prevDown[vk] = down;
	}

	// 案内が現れる前(90フレーム=約1.5秒)と、何も押されていない時は何もしない
	// ※ResultTextureの案内が現れる時間(90)と同じ値にしておく
	if (s_frame < 90 || !pressed) { return; }

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
