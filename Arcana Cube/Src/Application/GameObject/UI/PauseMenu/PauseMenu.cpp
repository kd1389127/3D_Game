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
	m_tex[kTexPauseTitle].Load("Asset/Textures/UI/Pause/PauseTitle_Menu.png");
	m_tex[kTexTitleLine].Load("Asset/Textures/UI/Pause/PauseTitleLine.png");
	m_tex[kTexCornerLine].Load("Asset/Textures/UI/Pause/CornerLine.png");
	m_tex[kTexHoverBar].Load("Asset/Textures/UI/Pause/HoverBar.png");
	m_tex[kTexRestart].Load("Asset/Textures/UI/Pause/Restart.png");
	m_tex[kTexStage].Load("Asset/Textures/UI/Pause/StageSelect.png");
	m_tex[kTexTitle].Load("Asset/Textures/UI/Pause/Title.png");
	m_tex[kTexArrowLeft].Load("Asset/Textures/UI/Icon/Arrow_Left.png");
	m_tex[kTexArrowRight].Load("Asset/Textures/UI/Icon/Arrow_Right.png");
	m_tex[kTexBackIcon].Load("Asset/Textures/UI/Icon/BackIcon.png");
	m_tex[kTexLock].Load("Asset/Textures/UI/Icon/Lock.png");
	m_tex[kTexDiamond].Load("Asset/Textures/UI/Icon/Diamond.png");
	m_tex[kTexPhotoFrame].Load("Asset/Textures/UI/Stage/PhotoFrame.png");

	// HowTo_Topic1.png〜HowTo_Topic3.png をまとめて読む
	for (int i = 0; i < kTopicCount; ++i)
	{
		m_tex[kTexTopic1 + i].Load("Asset/Textures/UI/Pause/HowTo_Topic" + std::to_string(i + 1) + ".png");
	}

	// Stage1.png〜StageN.png(N = 選べるステージの数)と、その写真・名前をまとめて読む
	for (int i = 0; i < kSelectableStageCount; ++i)
	{
		const std::string number = std::to_string(i + 1);
		m_tex[kTexStage1 + i].Load("Asset/Textures/UI/Pause/Stage" + number + ".png");
		m_tex[kTexPhoto1 + i].Load("Asset/Textures/UI/Stage/StagePhoto" + number + ".png");
		m_tex[kTexStageName1 + i].Load("Asset/Textures/UI/StageName/StageName" + number + ".png");	// ←フォルダは仮
	}

	SetPage(kPageMain);
}

void PauseMenu::SetOpen(bool _isOpen, bool _keepMouse)
{
	m_isOpen = _isOpen;
	m_hover  = -1;

	if (_isOpen) { SetPage(kPageMain); }

	// スライドへ移る時は、固定を切り替えない(中央に飛ばされるのを防ぐ)
	if (!_keepMouse) { Mouse::Instance().ShowMouse(_isOpen); }
}

PauseMenu::Button& PauseMenu::AddButton(int _texId, float _x, float _y, float _width, float _height)
{
	assert(m_count < kMaxBtn);				// 配列からはみ出す数を足したら、ここで気づけるようにする

	Button& button = m_buttons[m_count];
	button = Button();						// 前のページの設定(未解放や帯の印)が残らないよう、まず初期値に戻す
	button.texId  = _texId;
	button.pos    = Math::Vector2(_x, _y);
	button.width  = _width;
	button.height = _height;
	++m_count;
	return button;
}

float PauseMenu::GetRowY(int _row, int _rowCount)
{
	const float topY = (_rowCount - 1) * kStep * 0.5f;
	return topY - _row * kStep;
}

// 帯を描く。左右の角は元の大きさのまま、真ん中だけ伸ばす(角丸が歪まない)
void PauseMenu::DrawHoverBar(float _centerX, float _y, float _halfW, const Math::Color& _color)
{
	auto& sp = KdShaderManager::Instance().m_spriteShader;
	const KdTexture* tex = &m_tex[kTexHoverBar];

	const int   barH    = kBarHalfH * 2;
	const float capHalf = kBarCapW * 0.5f;
	const int   middleW = static_cast<int>(_halfW * 2) - kBarCapW * 2;	// 角を除いた真ん中の幅

	// 画像のどの部分を使うか(左の角・真ん中・右の角)
	const Math::Rectangle leftSrc   = { 0,                    0, kBarCapW,                barH };
	const Math::Rectangle middleSrc = { kBarCapW,             0, kBarTexW - kBarCapW * 2, barH };
	const Math::Rectangle rightSrc  = { kBarTexW - kBarCapW,  0, kBarCapW,                barH };

	sp.DrawTex(tex, _centerX - _halfW + capHalf, _y, kBarCapW, barH, &leftSrc, &_color);
	sp.DrawTex(tex, _centerX, _y, middleW, barH, &middleSrc, &_color);
	sp.DrawTex(tex, _centerX + _halfW - capHalf, _y, kBarCapW, barH, &rightSrc, &_color);
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
		// 中央に縦に3つ。マウスが乗ったら青い帯が出る
		const int ids[]    = { kTexResume, kTexRestart, kTexStage };
		const int rowCount = static_cast<int>(std::size(ids));
		for (int row = 0; row < rowCount; ++row)
		{
			Button& button  = AddButton(ids[row], 0.0f, GetRowY(row, rowCount));
			button.hasFrame = true;
		}

		// 左下に操作説明、右下にタイトルへ
		AddButton(kTexHowTo, -kCornerX, kCornerY, kCornerW, kCornerH);
		AddButton(kTexTitle, kCornerX, kCornerY, kCornerW, kCornerH);
		break;
	}
	case kPageStage:
	{
		// 写真(中央)。解放済みより先のステージは選べない(メニューを開いている間は解放は変わらない)
		Button& photo = AddButton(kTexPhoto1 + m_selectStage, 0.0f, kPhotoY, kPhotoW, kPhotoH);
		photo.locked  = (m_selectStage > SceneManager::Instance().GetUnlockedStage());

		// 左右の矢印と、左下の「戻る」
		AddButton(kTexArrowLeft, -kArrowX, kPhotoY, kArrowSize, kArrowSize);
		AddButton(kTexArrowRight, kArrowX, kPhotoY, kArrowSize, kArrowSize);
		AddButton(kTexBack, -kCornerX, kCornerY, kCornerW, kCornerH);
		break;
	}
	case kPageHowTo:
	{
		// 操作説明の項目(生成・削除 / 運ぶ・置く / ギミック)を中央に縦に並べ、「戻る」は左下
		for (int i = 0; i < kTopicCount; ++i)
		{
			Button& button  = AddButton(kTexTopic1 + i, 0.0f, GetRowY(i, kTopicCount));
			button.hasFrame = true;
		}
		AddButton(kTexBack, -kCornerX, kCornerY, kCornerW, kCornerH);
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

	if (!m_isOpen)      { return; }
	if (m_openT < 1.0f) { return; }	// 開ききるまでは、動いているボタンを押させない

	// マウスがどのボタンの上にあるか(GetScreenPosは中央が原点・上がプラスで、描画と同じ座標)
	const Math::Vector2 mouse = Mouse::Instance().GetScreenPos();
	m_hover = -1;
	for (int i = 0; i < m_count; ++i)
	{
		const Button& button = m_buttons[i];
		if (button.locked) { continue; }	// 未解放のボタンには反応しない

		const bool isInsideX = std::fabs(mouse.x - button.pos.x) <= button.width / 2;
		const bool isInsideY = std::fabs(mouse.y - button.pos.y) <= button.height / 2;
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
		SetOpen(false, true);

		if (spGuide) { spGuide->OpenSlides(topic, true); }

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
		{
			SetOpen(false);
			break;
		}
		case kTexHowTo:		// 操作説明の項目選びのページへ
		{
			SetPage(kPageHowTo);
			break;
		}
		case kTexRestart:	// リスタート(今のステージをもう一度)
		{
			SceneManager::Instance().RequestChangeScene(
				SceneManager::SceneType::Game, SceneManager::Instance().GetCurrentStage());
			break;
		}
		case kTexStage:		// ステージ選択のページへ(今遊んでいるステージの写真から始める)
		{
			m_selectStage = std::min(SceneManager::Instance().GetCurrentStage(), kSelectableStageCount - 1);
			SetPage(kPageStage);
			break;
		}
		case kTexArrowLeft:	// 前のステージの写真へ(先頭の次は最後に戻る)
		{
			m_selectStage = (m_selectStage + kSelectableStageCount - 1) % kSelectableStageCount;
			SetPage(kPageStage);
			break;
		}
		case kTexArrowRight:// 次のステージの写真へ(最後の次は先頭に戻る)
		{
			m_selectStage = (m_selectStage + 1) % kSelectableStageCount;
			SetPage(kPageStage);
			break;
		}
		case kTexTitle:		// タイトルへ
		{
			SceneManager::Instance().RequestChangeScene(SceneManager::SceneType::Title, 0);
			break;
		}
		case kTexBack:		// メインのページへ戻る
		{
			SetPage(kPageMain);
			break;
		}
		default:			// ステージの写真(画像番号 - kTexPhoto1 がステージ番号)
		{
			SceneManager::Instance().RequestChangeScene(
				SceneManager::SceneType::Game, id - kTexPhoto1);
			break;
		}
	}
}

void PauseMenu::DrawSprite()
{
	// 閉じる動きが終わるまでは描く(!m_isOpen ではなく m_openT で判定する)
	if (m_openT <= 0.0f) { return; }

	// イージング：最初は速く、最後はゆっくり止まる動き
	const float e = KdEase::OutCubic(m_openT);

	// 下から上がってくる距離(eが0の時は下、1の時は元の位置)。全部の絵で共通
	const float slideUp = (1.0f - e) * kSlideUpDistance;

	auto& sp = KdShaderManager::Instance().m_spriteShader;

	// 画面全体を暗くする(暗さも開き具合に合わせて濃くする)
	Math::Color dim = { 0.0f, 0.0f, 0.0f, 0.6f * e };
	sp.DrawBox(0, 0, 800, 450, &dim, true);

	// 明るさを変えない絵(鍵・枠・帯・ステージ名など)の色
	const Math::Color plain = { 1.0f, 1.0f, 1.0f, e };

	for (int i = 0; i < m_count; ++i)
	{
		const Button& button = m_buttons[i];

		// 未解放はかなり暗く、マウスが乗っているボタンは明るく、他は少し暗くする
		float brightness = kBrightnessNormal;
		if (i == m_hover) { brightness = kBrightnessHover; }
		if (button.locked) { brightness = kBrightnessLocked; }
		Math::Color color = { brightness, brightness, brightness, e };	// 透明→不透明

		const float y = button.pos.y - slideUp;

		// 帯のあるボタンにマウスが乗ったら、青い帯と左右の◆を描く(文字より後ろに描く)
		if (i == m_hover && button.hasFrame)
		{
			const float diamondOffsetX = kBarHalfW - kHoverDiamondInset;	// 帯の中心から◆の中心まで

			DrawHoverBar(button.pos.x, y, kBarHalfW, plain);
			sp.DrawTex(&m_tex[kTexDiamond], button.pos.x - diamondOffsetX, y,
				kHoverDiamondSize, kHoverDiamondSize, nullptr, &plain);
			sp.DrawTex(&m_tex[kTexDiamond], button.pos.x + diamondOffsetX, y,
				kHoverDiamondSize, kHoverDiamondSize, nullptr, &plain);
		}

		sp.DrawTex(&m_tex[button.texId], button.pos.x, y, button.width, button.height, nullptr, &color);

		// 左下の「操作説明」と右下の「タイトルへ」には、下線を添える
		if (button.texId == kTexHowTo || button.texId == kTexTitle)
		{
			sp.DrawTex(&m_tex[kTexCornerLine], button.pos.x, y + kCornerLineOffsetY,
				kCornerLineW, kCornerLineH, nullptr, &color);
		}

		// 未解放のボタンには鍵を重ねる
		if (button.locked)
		{
			sp.DrawTex(&m_tex[kTexLock], button.pos.x, y, kLockSize, kLockSize, nullptr, &plain);
		}

		// 「戻る」だけ、左にアイコンを添える
		if (button.texId == kTexBack)
		{
			sp.DrawTex(&m_tex[kTexBackIcon], button.pos.x + kBackIconOffsetX, y,
				kBackIconSize, kBackIconSize, nullptr, &color);
		}
	}

	// メインのページだけ、上に「ポーズ」と飾り線
	if (m_page == kPageMain)
	{
		sp.DrawTex(&m_tex[kTexPauseTitle], kPauseTitleX, kPauseTitleY - slideUp, kBtnW, kBtnH, nullptr, &plain);
		sp.DrawTex(&m_tex[kTexTitleLine], 0.0f, kPauseTitleLineY - slideUp,
			kPauseTitleLineW, kPauseTitleLineH, nullptr, &plain);
	}

	// ここから下は、ステージ選択のページだけ
	if (m_page != kPageStage) { return; }

	// 写真の枠(写真より手前に描く)
	sp.DrawTex(&m_tex[kTexPhotoFrame], 0.0f, kPhotoY - slideUp, kFrameW, kFrameH, nullptr, &plain);

	// ステージ名(〜 封印の間 〜)と、「ステージ n」
	sp.DrawTex(&m_tex[kTexStageName1 + m_selectStage], 0.0f, kNameY - slideUp, kBtnW, kBtnH, nullptr, &plain);
	sp.DrawTex(&m_tex[kTexStage1 + m_selectStage], 0.0f, kLabelY - slideUp, kBtnW, kBtnH, nullptr, &plain);

	// 下の点(ダイヤ)。今のステージだけ大きく明るくする
	const float dotsLeft = -(kSelectableStageCount - 1) * kDotStep * 0.5f;	// 点の列が画面の中央に来る左端
	for (int i = 0; i < kSelectableStageCount; ++i)
	{
		float dotSize       = kDotSize;
		float dotBrightness = kBrightnessNormal;
		if (i == m_selectStage)
		{
			dotSize = kDotSelectedSize;
			dotBrightness = kBrightnessHover;
		}

		const Math::Color dotColor = { dotBrightness, dotBrightness, dotBrightness, e };
		sp.DrawTex(&m_tex[kTexDiamond], dotsLeft + i * kDotStep, kDotsY - slideUp,
			dotSize, dotSize, nullptr, &dotColor);
	}
}