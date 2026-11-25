#include "Guide.h"
#include "../Mouse/Mouse.h"
#include "../../../Scene/SceneManager.h"
#include "../../../Scene/StageData.h"

void Guide::Init()
{
	// ヒントの数が StageDataのHintKindと食い違っていたらビルドで止める
	static_assert(kHintCount == static_cast<int>(HintKind::Count),
		"ヒントの数が Guide と HintKind で違います");

	m_drawType = eDrawTypeUI;

	const std::string dir = "Asset/Textures/UI/";

	// 項目ごとのスライドのファイル名。並べた順に表示される
	// 枚数を増やす時は、ここに足すだけでよい
	const std::vector<std::string> fileNames[kAllTopicCount] =
	{
		// 操作説明
		// 0 生成・削除(Tabメニュー)
		{ 
			"Guide/Slide1.png", 
			"Guide/Slide2.png", 
			"Guide/Slide3.png",
			"Guide/Slide4.png",
			"Guide/Slide5.png" 
		},
		// 1 運ぶ・置く(Tabメニュー)
		{ 
			"Guide/Grab.png",
			"Guide/Place.png" 
		},
		// 2 ギミック(Tabメニュー)
		{ 
			"Guide/Switch.png",
			"Guide/Cage.png", 
			"Guide/Lift.png", 
			"Guide/Wall.png" 
		},
		
		// ヒント
		// 3 生成・魔力・削除(ステージ2)
		{ 
			"Hint/Slide1.png",
			"Hint/Slide2.png",
			"Hint/Slide3.png",
			"Hint/Slide4.png",
			"Hint/Slide5.png" 
		},
		// 4 運ぶ・置く・スイッチ・檻(ステージ3)
		{ 
			"Hint/Grab.png",
			"Hint/Place.png",
			"Hint/Switch.png",
			"Hint/Cage.png" 
		},
		// 5 生成できない壁(ステージ5)
		{ "Hint/Wall.png" },
		// 6 リフト(ステージ6)
		{ "Hint/Lift.png" },
	};

	for (int topic = 0; topic < kAllTopicCount; ++topic)
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

void Guide::OpenSlides(int _topic, bool _keepMouse)
{
	// 範囲外の項目や、画像が1枚もない項目は開かない
	const bool isValidTopic = (_topic >= 0 && _topic < kAllTopicCount);
	if (!isValidTopic) { return; }
	if (m_slideCount[_topic] == 0) { return; }

	m_topic = _topic;
	m_slideIndex = 0;
	m_keepMouseOnClose = _keepMouse;
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

		// メニューから開いた時は、このあとメニューがマウスを使うので固定しない
		if (!m_keepMouseOnClose)
		{
			Mouse::Instance().ShowMouse(false);
		}
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