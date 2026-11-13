#include "Guide.h"
#include "../Mouse/Mouse.h"
#include "../../../Scene/SceneManager.h"

void Guide::Init()
{
	m_drawType = eDrawTypeUI;

	const std::string dir = "Asset/Textures/UI/Guide/";

	// 項目ごとのスライドのファイル名。並べた順に表示される
	// 枚数を増やす時は、ここに足すだけでよい
	const std::vector<std::string> fileNames[kTopicCount] =
	{
		// 0 生成・削除(5枚)
		{
			"Slide1.png",
			"Slide2.png",
			"Slide3.png",
			"Mana.png",
			"Erase.png",
		},
		// 1 運ぶ・置く(2枚)
		{
			"Grab.png",
			"Place.png",
		},
		// 2 ギミック(4枚)
		{
			"Switch.png",
			"Cage.png",
			"Lift.png",
			"Wall.png",
		},
	};

	for (int topic = 0; topic < kTopicCount; ++topic)
	{
		const auto& names = fileNames[topic];

		// 入りきらない枚数を書いてしまったら、ここで気づけるようにする
		assert(static_cast<int>(names.size()) <= kMaxSlides);

		m_slideCount[topic] = static_cast<int>(names.size());

		int slide = 0;
		for (const auto& name : names)
		{
			m_slideTex[topic][slide].Load(dir + name);
			++slide;
		}
	}
}

void Guide::OpenSlides(int _topic)
{
	// 範囲外の項目や、画像が1枚もない項目は開かない
	const bool isValidTopic = (_topic >= 0 && _topic < kTopicCount);
	if (!isValidTopic) { return; }
	if (m_slideCount[_topic] == 0) { return; }

	m_topic = _topic;
	m_slideIndex = 0;
	Mouse::Instance().ShowMouse(true);	// クリックで進めるのでポインタを出す
}

void Guide::Update()
{
	if (m_openWait > 0)
	{
		--m_openWait;
		if (m_openWait == 0) { OpenSlides(); }
		return;
	}

	if (m_slideIndex < 0) { return; }						// スライドを出していない時は何もしない

	if (SceneManager::Instance().IsFading()) { return; }	// 暗転中は進めない
	if (!Mouse::Instance().IsClick()) { return; }

	++m_slideIndex;
	if (m_slideIndex >= GetSlideCount())					// その項目の最後の1枚で押したら閉じる
	{
		m_slideIndex = -1;
		Mouse::Instance().ShowMouse(false);
	}
}

void Guide::DrawSprite()
{
	auto& sp = KdShaderManager::Instance().m_spriteShader;

	// 操作説明スライド(一番手前。周りは暗くしたゲーム画面のまま)
	if (m_slideIndex < 0) { return; }

	Math::Color dim = { 0.0f, 0.0f, 0.0f, kDimAlpha };
	sp.DrawBox(0, 0, 800, 450, &dim, true);
	sp.DrawTex(&m_slideTex[m_topic][m_slideIndex], 0, 0, kSlideW, kSlideH);
}