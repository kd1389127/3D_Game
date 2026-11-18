#pragma once
//============================================================
// マウスクラス(シングルトン)
//	Mouse::Instance() でどのシーンからでも使える
//
//	ShowMouse(true)  : 杖を描く + マウスを固定しない(タイトル/メニュー/リザルト用)
//	ShowMouse(false) : 杖を描かない + マウスを画面中央に固定(ゲーム本編用)
//============================================================
class Mouse
{
public:
	void Init();
	void Update();
	void Draw();

	// 画面中央が原点、上がプラスのスクリーン座標
	Math::Vector2 GetScreenPos() { return m_screenPos; }

	// 現在のフレームのマウスの移動量を返す
	Math::Vector2 GetDelta() { return m_delta; }

	// クリックしたスクリーン座標を返す
	Math::Vector2 GetClickPos() { return m_clickPos; }

	// 押した瞬間(一度離してから押したときだけtrue)
	bool IsClick() { return m_leftClick.m_isClick; }

	// 押している間
	bool IsPush() { return m_leftClick.m_isPush; }

	bool IsRightClick() { return m_rightClick.m_isClick; }
	bool IsRightPush() { return m_rightClick.m_isPush; }

	void ResetCursorToCenter();

	Math::Vector2 GetMouseDeletaNormalized() { return m_normalizedDelta; }

	//　true:杖を表示して固定しない / false:杖を隠して中央に固定する
	void ShowMouse(bool _isShow);

	// 本編中だけ一時的にカーソルの固定を解く(スクショ撮影などで自由に動かしたい時用)
	void SetFreeMove(bool _isFree);

private:

	void MousePos();
	void ClampGameMousePos();

	struct MouseClick
	{
		bool m_isClick = false;
		bool m_isPush = false;

		const int kIntervalTime = 10;
		int m_clickInterval = 0;
		bool m_canClick = true;
		bool m_isDown = false;
	};

	void Click(MouseClick& _click, bool _isLeft);

	MouseClick m_leftClick;
	MouseClick m_rightClick;

	static constexpr int kScrWidth = 1280;
	static constexpr int kScrHeight = 720;

	POINT m_mousePos = { 0,0 };
	POINT m_prevPos = { 0,0 };

	Math::Vector2 m_gameMouse = Math::Vector2::Zero;
	Math::Vector2 m_delta = Math::Vector2::Zero;
	Math::Vector2 m_clickPos = Math::Vector2::Zero;
	Math::Vector2 m_screenPos = Math::Vector2::Zero;

	Math::Vector2 m_normalizedDelta = { 0,0 };
	const float kMaxDelta = 30.0f;
	const float kSensitivity = 1.0f;

	bool m_isShowMouse = true;

	//マウス画像(杖)
	static constexpr int kImageSize = 64;
	std::shared_ptr<KdTexture> m_mouseTex;

	bool m_isFreeMove = false;

//すべてのシーンでマウスをつかうためシングルトン
private:
	Mouse() = default;
	~Mouse() = default;

	Mouse(Mouse&) = delete;
	Mouse& operator = (Mouse&) = delete;
public:
	static Mouse& Instance() {
		static Mouse instance;
		return instance;
	}
};