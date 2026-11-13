#include "PauseMenu.h"
#include "../Mouse/Mouse.h"
#include "../Guide/Guide.h"
#include "../../../Scene/SceneManager.h"
#include "../../../Scene/StageData.h"

void PauseMenu::Init()
{
	// 項目の数が Guide と食い違っていたらビルドで止める
	static_assert(kTopicCount == Guide::kTopicCount, "項目の数が PauseMenu と Guide で違います");

	m_tex[kTexResume].Load("Asset/Textures/UI/Pause/Resume.png");
	m_tex[kTexHowTo].Load("Asset/Textures/UI/Pause/HowTo_Controls.png");
	m_tex[kTexBack].Load("Asset/Textures/UI/Pause/Back.png");
	m_tex[kTexRestart].Load("Asset/Textures/UI/Pause/Restart.png");
	m_tex[kTexStage].Load("Asset/Textures/UI/Pause/StageSelect.png");
	m_tex[kTexTitle].Load("Asset/Textures/UI/Pause/Title.png");

	// HowTo_Topic1.png〜HowTo_Topic3.png をまとめて読む
	for (int i = 0; i < kTopicCount; ++i)
	{
		m_tex[kTexTopic1 + i].Load("Asset/Textures/UI/Pause/HowTo_Topic" + std::to_string(i + 1) + ".png");
	}

	// Stage1.png〜Stage5.png をまとめて読む
	for (int i = 0; i < kStageCount; ++i)
	{
		m_tex[kTexStage1 + i].Load("Asset/Textures/UI/Pause/Stage" + std::to_string(i + 1) + ".png");
	}

	SetPage(kPageMain);
}

void PauseMenu::SetOpen(bool _isOpen)
{
	m_isOpen = _isOpen;
	m_hover = -1;

	// 開くときだけメインに戻す(閉じる動きの最中に、ボタンの並びが変わらないように)
	if (_isOpen) { SetPage(kPageMain); }

	// 開いている間は杖のポインタを出して固定を解き、閉じたら中央固定に戻す
	Mouse::Instance().ShowMouse(_isOpen);
}

void PauseMenu::SetPage(Page _page)
{
	m_page = _page;
	m_hover = -1;

	for (bool& locked : m_locked) { locked = false; }

	switch (_page)
	{
	case kPageMain:
	{
		const int ids[] = { kTexResume, kTexHowTo, kTexRestart, kTexStage, kTexTitle };
		m_count = static_cast<int>(std::size(ids));
		for (int i = 0; i < m_count; ++i) { m_ids[i] = ids[i]; }
		break;
	}
	case kPageStage:
	{
		// STAGE 1〜5 と、いちばん下に BACK
		for (int i = 0; i < kStageCount; ++i)
		{
			m_ids[i] = kTexStage1 + i;
			// 解放済みより先のステージは選べない(メニューを開いている間は解放は変わらない)
			m_locked[i] = (kStageMap1 + i > SceneManager::Instance().GetUnlockedStage());
		}
		m_ids[kStageCount] = kTexBack;
		m_count = kStageCount + 1;
		break;
	}
	case kPageHowTo:
	{
		// 操作説明の項目(生成・削除 / 運ぶ・置く / ギミック)と、いちばん下に BACK
		for (int i = 0; i < kTopicCount; ++i)
		{
			m_ids[i] = kTexTopic1 + i;
		}
		m_ids[kTopicCount] = kTexBack;
		m_count = kTopicCount + 1;
		break;
	}
	}
}

void PauseMenu::Update()
{
	// 開き具合を、開いていれば1へ、閉じていれば0へ、毎フレーム少しずつ近づける
	const float target = m_isOpen ? 1.0f : 0.0f;
	if (m_openT < target) { m_openT = std::min(m_openT + kOpenStep, 1.0f); }
	else if (m_openT > target) { m_openT = std::max(m_openT - kOpenStep, 0.0f); }

	// Tabは「押した瞬間」だけ反応させる(押しっぱなしで開閉を繰り返さないため)
	const bool nowTab = (GetAsyncKeyState(VK_TAB) & 0x8000) != 0;
	const bool tabPressed = nowTab && !m_prevTab;
	m_prevTab = nowTab;	// returnより前に更新する(後ろに置くと押しっぱなしを覚えられない)

	// 暗転中(ゴール・奈落・リスタート中)は開閉もクリックも受け付けない
	if (SceneManager::Instance().IsFading()) { return; }

	// スライドを見ている間はTabを受け付けない(スライドのクリックとメニューのクリックが混ざらないように)
	const std::shared_ptr<Guide> spGuide = m_wpGuide.lock();
	const bool isSlideOpen = spGuide && spGuide->IsSlideOpen();

	// 項目のスライドを見終わったら、操作説明の項目選びのページに戻る
	if (m_returnToHowTo && !isSlideOpen)
	{
		m_returnToHowTo = false;
		SetOpen(true);
		SetPage(kPageHowTo);	// SetOpen は必ずメインに戻すので、その後で項目選びに切り替える
		return;
	}

	if (tabPressed && !isSlideOpen)
	{
		SetOpen(!m_isOpen);
		return;
	}

	if (!m_isOpen) { return; }
	if (m_openT < 1.0f) { return; }	// 開ききるまでは、動いているボタンを押させない

	// マウスがどのボタンの上にあるか(GetScreenPosは中央が原点・上がプラスで、描画と同じ座標)
	// ボタンはページごとに数が違うので、全体が画面の中央に来るように一番上のYを決める
	const Math::Vector2 mouse = Mouse::Instance().GetScreenPos();
	const float topY = (m_count - 1) * kStep * 0.5f;
	m_hover = -1;
	for (int i = 0; i < m_count; ++i)
	{
		if (m_locked[i]) { continue; }	// 未解放のボタンには反応しない

		const float y = topY - i * kStep;
		if (std::fabs(mouse.x) <= kBtnW / 2 && std::fabs(mouse.y - y) <= kBtnH / 2)
		{
			m_hover = i;
		}
	}

	if (m_hover < 0 || !Mouse::Instance().IsClick()) { return; }

	auto se = KdAudioManager::Instance().Play("Asset/Sounds/SE/UI/Click.wav", false);
	if (se) { se->SetVolume(0.15f); }

	// 押したボタンに応じた処理(ボタンの画像番号で判断する)
	const int id = m_ids[m_hover];

	// 操作説明の項目を押した：メニューを閉じて、その項目のスライドを出す
	const bool isTopicButton = (id >= kTexTopic1 && id < kTexTopic1 + kTopicCount);
	if (isTopicButton)
	{
		const int topic = id - kTexTopic1;
		SetOpen(false);

		if (spGuide) { spGuide->OpenSlides(topic); }

		// スライドが実際に開いた時だけ、見終わったらここに戻る
		// (画像のない項目などで開かなかった時は、そのまま項目選びを開き直す)
		if (spGuide && spGuide->IsSlideOpen())
		{
			m_returnToHowTo = true;
		}
		else
		{
			SetOpen(true);
			SetPage(kPageHowTo);
		}
		return;
	}

	switch (id)
	{
	case kTexResume:	// 再開
		SetOpen(false);
		break;
	case kTexHowTo:		// 操作説明の項目選びのページへ
		SetPage(kPageHowTo);
		break;
	case kTexRestart:	// リスタート(今のステージをもう一度)
		SceneManager::Instance().RequestChangeScene(
			SceneManager::SceneType::Game, SceneManager::Instance().GetCurrentStage());
		break;
	case kTexStage:		// ステージ選択のページへ
		SetPage(kPageStage);
		break;
	case kTexTitle:		// タイトルへ
		SceneManager::Instance().RequestChangeScene(SceneManager::SceneType::Title, 0);
		break;
	case kTexBack:		// メインのページへ戻る
		SetPage(kPageMain);
		break;
	default:			// STAGE 1〜5(画像番号からステージ番号を求める。チュートリアルは選べない)
		SceneManager::Instance().RequestChangeScene(
			SceneManager::SceneType::Game, kStageMap1 + (id - kTexStage1));
		break;
	}
}

void PauseMenu::DrawSprite()
{
	// 閉じる動きが終わるまでは描く(!m_isOpen ではなく m_openT で判定する)
	if (m_openT <= 0.0f) { return; }

	// イージング：最初は速く、最後はゆっくり止まる動き
	const float e = KdEase::OutCubic(m_openT);

	auto& sp = KdShaderManager::Instance().m_spriteShader;

	// 画面全体を暗くする(暗さも開き具合に合わせて濃くする)
	Math::Color dim = { 0.0f, 0.0f, 0.0f, 0.6f * e };
	sp.DrawBox(0, 0, 800, 450, &dim, true);

	const float topY = (m_count - 1) * kStep * 0.5f;	// Updateと同じ式
	for (int i = 0; i < m_count; ++i)
	{
		// 未解放はかなり暗く、マウスが乗っているボタンは明るく、他は少し暗くする
		float c = (i == m_hover) ? 1.0f : 0.6f;
		if (m_locked[i]) { c = 0.25f; }
		Math::Color color = { c, c, c, e };	// 透明→不透明

		// 下から40だけ上がってくる(eが0の時は40下、1の時は元の位置)
		const float y = topY - i * kStep - (1.0f - e) * 40.0f;
		sp.DrawTex(&m_tex[m_ids[i]], 0, y, kBtnW, kBtnH, nullptr, &color);
	}
}