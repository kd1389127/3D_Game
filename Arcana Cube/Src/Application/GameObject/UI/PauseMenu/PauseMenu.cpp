#include "PauseMenu.h"
#include "../Mouse/Mouse.h"
#include "../Guide/Guide.h"
#include "../../../Scene/SceneManager.h"
#include "../../../Scene/StageData.h"

namespace
{
	// ステージ選択で選べるステージの数(最後のGoalMapは選ばせない)
	// g_stageTable から自動で求めるので、ステージを足してもここは直さなくてよい
	constexpr int kSelectableStageCount = g_stageCount - 1;
}

void PauseMenu::Init()
{
	// 項目の数が Guide と食い違っていたらビルドで止める
	static_assert(kTopicCount == Guide::kTopicCount, "項目の数が PauseMenu と Guide で違います");

	// ステージが10を超えたらビルドで止める(配列からはみ出して、バグになるのを防ぐ)
	static_assert(kSelectableStageCount <= kMaxStageCount,
		"ステージが多すぎます。ステージ選択のページ送りが必要です");

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

	// Stage1.png〜StageN.png(N = 選べるステージの数)をまとめて読む
	for (int i = 0; i < kSelectableStageCount; ++i)
	{
		m_tex[kTexStage1 + i].Load("Asset/Textures/UI/Pause/Stage" + std::to_string(i + 1) + ".png");
	}

	SetPage(kPageMain);
}

void PauseMenu::SetOpen(bool _isOpen, bool _keepMouse)
{
	m_isOpen = _isOpen;
	m_hover = -1;

	if (_isOpen) { SetPage(kPageMain); }

	// スライドへ移る時は、固定を切り替えない(中央に飛ばされるのを防ぐ)
	if (!_keepMouse) { Mouse::Instance().ShowMouse(_isOpen); }
}

PauseMenu::Button& PauseMenu::AddButton(int _texId, float _x, float _y, float _hitWidth)
{
	assert(m_count < kMaxBtn);	// 配列からはみ出す数を足したら、ここで気づけるようにする

	Button& button = m_buttons[m_count];
	button.texId = _texId;
	button.pos = Math::Vector2(_x, _y);
	button.hitWidth = _hitWidth;
	button.locked = false;
	++m_count;
	return button;
}

float PauseMenu::GetRowY(int _row, int _rowCount)
{
	const float topY = (_rowCount - 1) * kStep * 0.5f;
	return topY - _row * kStep;
}

void PauseMenu::SetPage(Page _page)
{
	m_page = _page;
	m_hover = -1;
	m_count = 0;

	switch (_page)
	{
	case kPageMain:
	{
		const int ids[] = { kTexResume, kTexHowTo, kTexRestart, kTexStage, kTexTitle };
		const int rowCount = static_cast<int>(std::size(ids));
		for (int row = 0; row < rowCount; ++row)
		{
			AddButton(ids[row], 0.0f, GetRowY(row, rowCount));
		}
		break;
	}
	case kPageStage:
	{
		// 行数は、ステージが少ない時でも「ステージの行 + 戻る」で固定する(「戻る」の位置を動かさないため)
		const int rowCount = kStageRows + 1;

		// 列の数(端数が出たら切り上げる)。8ステージなら、(8 + 4) / 5 = 2列
		const int stageColumns = (kSelectableStageCount + kStageRows - 1) / kStageRows;

		for (int stage = 0; stage < kSelectableStageCount; ++stage)
		{
			const int column = stage / kStageRows;	// 0=左の列 1=右の列
			const int row = stage % kStageRows;

			// 列の並びが画面の中央に来るようにずらす(1列ならX=0、2列なら左右に半分ずつ)
			const float columnOffset = column - (stageColumns - 1) * 0.5f;
			const float x = columnOffset * kStageColumnStep;

			Button& button = AddButton(kTexStage1 + stage, x, GetRowY(row, rowCount), kStageColumnStep);

			// 解放済みより先のステージは選べない(メニューを開いている間は解放は変わらない)
			button.locked = (stage > SceneManager::Instance().GetUnlockedStage());
		}

		// いちばん下の真ん中に「戻る」
		AddButton(kTexBack, 0.0f, GetRowY(kStageRows, rowCount));
		break;
	}
	case kPageHowTo:
	{
		// 操作説明の項目(生成・削除 / 運ぶ・置く / ギミック)と、いちばん下に BACK
		const int rowCount = kTopicCount + 1;
		for (int i = 0; i < kTopicCount; ++i)
		{
			AddButton(kTexTopic1 + i, 0.0f, GetRowY(i, rowCount));
		}
		AddButton(kTexBack, 0.0f, GetRowY(kTopicCount, rowCount));
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
	const Math::Vector2 mouse = Mouse::Instance().GetScreenPos();
	m_hover = -1;
	for (int i = 0; i < m_count; ++i)
	{
		const Button& button = m_buttons[i];
		if (button.locked) { continue; }	// 未解放のボタンには反応しない

		const bool isInsideX = std::fabs(mouse.x - button.pos.x) <= button.hitWidth / 2;
		const bool isInsideY = std::fabs(mouse.y - button.pos.y) <= kBtnH / 2;
		if (isInsideX && isInsideY)
		{
			m_hover = i;
		}
	}

	if (m_hover < 0 || !Mouse::Instance().IsClick()) { return; }

	auto se = KdAudioManager::Instance().Play("Asset/Sounds/SE/UI/Click.wav", false);
	if (se) { se->SetVolume(0.15f); }

	// 押したボタンに応じた処理(ボタンの画像番号で判断する)
	const int id = m_buttons[m_hover].texId;

	// 操作説明の項目を押した：メニューを閉じて、その項目のスライドを出す
	const bool isTopicButton = (id >= kTexTopic1 && id < kTexTopic1 + kTopicCount);
	if (isTopicButton)
	{
		const int topic = id - kTexTopic1;
		SetOpen(false,true);

		if (spGuide) { spGuide->OpenSlides(topic,true); }

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
	default:			// ステージのボタン(画像番号がそのままステージ番号。0=チュートリアル)
		SceneManager::Instance().RequestChangeScene(
			SceneManager::SceneType::Game, id - kTexStage1);
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

	for (int i = 0; i < m_count; ++i)
	{
		const Button& button = m_buttons[i];

		// 未解放はかなり暗く、マウスが乗っているボタンは明るく、他は少し暗くする
		float brightness = kBrightnessNormal;
		if (i == m_hover) { brightness = kBrightnessHover; }
		if (button.locked) { brightness = kBrightnessLocked; }
		Math::Color color = { brightness, brightness, brightness, e };	// 透明→不透明

		// 下から kSlideUpDistance だけ上がってくる(eが0の時は下、1の時は元の位置)
		const float y = button.pos.y - (1.0f - e) * kSlideUpDistance;
		sp.DrawTex(&m_tex[button.texId], button.pos.x, y, kBtnW, kBtnH, nullptr, &color);
	}
}