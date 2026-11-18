#include "Mouse.h"
#include "../../../main.h"

#include <algorithm>

void Mouse::Init()
{
	ResetCursorToCenter();

	m_gameMouse.x = 0.0f;
	m_gameMouse.y = 0.0f;

	// 杖の画像
	m_mouseTex = std::make_shared<KdTexture>();
	m_mouseTex->Load("Asset/Textures/UI/Mouse/Cursor.png");
}

void Mouse::Update()
{
	MousePos();
	Click(m_leftClick, true);
	Click(m_rightClick, false);
}

void Mouse::Draw()
{
	// 本編(固定中)では杖を描かない
	if (!m_isShowMouse) return;
	if (!m_mouseTex) return;

	auto& sp = KdShaderManager::Instance().m_spriteShader;
	sp.Begin();
	sp.SetMatrix(Math::Matrix::Identity);

	// 画像の中で杖の先端がある位置(左上を(0,0)、右下を(1,1)とした割合)
	// 見ながら調整する。先端がマウス位置からずれて見えたらここを変える
	const float tipX = 0.17f;
	const float tipY = 0.06f;

	// 画像の中心をどれだけずらせば、先端がマウス位置に来るか
	// (画面はY上向きがプラスなので、下に動かす分はマイナス)
	const float offsetX = kImageSize * (0.5f - tipX);
	const float offsetY = -kImageSize * (0.5f - tipY);

	sp.DrawTex(m_mouseTex,
		(int)(GetScreenPos().x + offsetX),
		(int)(GetScreenPos().y + offsetY),
		kImageSize, kImageSize);

	sp.End();
}

void Mouse::ResetCursorToCenter()
{
	if (m_isShowMouse || m_isFreeMove) return;

	HWND hwnd = Application::Instance().GetWindowHandle();
	RECT rect;
	GetWindowRect(hwnd, &rect);
	int x = static_cast<int>(rect.left + kScrWidth / 2);
	int y = static_cast<int>(rect.top + kScrHeight / 2);
	m_prevPos = { x,y };
	SetCursorPos(x, y);

}

void Mouse::ShowMouse(bool _isShow)
{
	m_isShowMouse = _isShow;

	// 固定に切り替えた瞬間に中央へ戻して、deltaの飛びを防ぐ
	if (!m_isShowMouse)
	{
		ResetCursorToCenter();
		m_delta = Math::Vector2::Zero;
		m_normalizedDelta = Math::Vector2::Zero;
	}
}

void Mouse::SetFreeMove(bool _isFree)
{
	if (m_isFreeMove == _isFree) return;
	m_isFreeMove = _isFree;

	// 固定に戻した瞬間に中央へ戻して、移動量(delta)が飛ぶのを防ぐ
	if (!m_isFreeMove)
	{
		ResetCursorToCenter();
		m_delta = Math::Vector2::Zero;
		m_normalizedDelta = Math::Vector2::Zero;
	}
}

void Mouse::MousePos()
{
	POINT currentCursorPos;
	GetCursorPos(&currentCursorPos);

	POINT clientPos = currentCursorPos;
	ScreenToClient(Application::Instance().GetWindowHandle(), &clientPos);
	m_screenPos.x = static_cast<float>(clientPos.x) - kScrWidth / 2;
	m_screenPos.y = -(static_cast<float>(clientPos.y) - kScrHeight / 2);

	float deltaX = static_cast<float>(currentCursorPos.x - m_prevPos.x);
	float deltaY = -static_cast<float>(currentCursorPos.y - m_prevPos.y);

	m_delta.x = deltaX;
	m_delta.y = deltaY;

	m_normalizedDelta.x = std::max(-1.0f, std::min(1.0f, deltaX / kMaxDelta));
	m_normalizedDelta.y = std::max(-1.0f, std::min(1.0f, deltaY / kMaxDelta));

	if (m_isFreeMove)
	{
		m_delta = Math::Vector2::Zero;
		m_normalizedDelta = Math::Vector2::Zero;
	}

	m_gameMouse.x += deltaX * kSensitivity;
	m_gameMouse.y += deltaY * kSensitivity;

	ClampGameMousePos();

	m_mousePos.x = static_cast<long>(m_gameMouse.x);
	m_mousePos.y = static_cast<long>(m_gameMouse.y);

	ResetCursorToCenter();
}

void Mouse::ClampGameMousePos()
{
	const float halfW = kScrWidth / 2.0f;
	const float halfH = kScrHeight / 2.0f;

	if (m_gameMouse.x < -halfW) m_gameMouse.x = -halfW;
	if (m_gameMouse.x > halfW)  m_gameMouse.x = halfW;
	if (m_gameMouse.y < -halfH) m_gameMouse.y = -halfH;
	if (m_gameMouse.y > halfH)  m_gameMouse.y = halfH;


}

void Mouse::Click(MouseClick & _click, bool _isLeft)
{
	_click.m_isClick = false;
	_click.m_isPush = false;

	if (_click.m_clickInterval > 0) { _click.m_clickInterval--; return; }
	_click.m_canClick = true;

	if (!_click.m_canClick) return;

	if (_isLeft) {
		if (!(GetAsyncKeyState(VK_LBUTTON) & 0x8000)) {
			_click.m_isDown = true;
			return;
		}
	}
	else {
		if (!(GetAsyncKeyState(VK_RBUTTON) & 0x8000)) {
			_click.m_isDown = true;
			return;
		}
	}
	_click.m_isPush = true;
	if (!_click.m_isDown) return;

	m_clickPos = GetScreenPos();

	_click.m_isClick = true;
	_click.m_canClick = false;
	_click.m_clickInterval = _click.kIntervalTime;
	_click.m_isDown = false;
}
