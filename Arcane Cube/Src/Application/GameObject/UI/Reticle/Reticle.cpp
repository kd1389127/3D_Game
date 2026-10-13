#include "Reticle.h"
#include "../../Character/Player/Player.h"
#include "../../Magic/MagicManager.h"

void Reticle::Init()
{
	m_texNormal.Load("Asset/Textures/Reticle/Reticle_Blue.png");
	m_texDelete.Load("Asset/Textures/Reticle/Reticle_Red.png");
	m_texGrab.Load("Asset/Textures/Reticle/Reticle_Green.png");
}

void Reticle::Update()
{
	// MagicManager側で失敗が起きるとIDが進むので、前回見た値と違えば振動を開始する
	int currentFailId = MagicManager::Instance().GetAttemptFailedId();
	if (currentFailId != m_lastSeenFailId)
	{
		m_lastSeenFailId = currentFailId;
		StartShake();
	}

	// 「離せなかった」時も、魔力切れと同じ振動で知らせる
	if (auto player = m_wpPlayer.lock())
	{
		const int dropId = player->GetDropBlockedId();
		if (dropId != m_lastSeenDropBlockedId)
		{
			m_lastSeenDropBlockedId = dropId;
			StartShake();
		}
	}

	if (!m_isShaking) return;

	m_shakeFrame++;
	float t = m_shakeFrame / (float)m_shakeDuration;

	if (t >= 1.0f)
	{
		m_isShaking = false;
		m_shakeOffset = Math::Vector2::Zero;
		m_shakeScale = 1.0f;
		return;
	}

	// だんだん収まる振動
	float decay = 1.0f - t;
	float wobble = sinf(t * DirectX::XM_PI * 8.0f) * decay;

	m_shakeOffset = Math::Vector2(wobble * 6.0f, 0.0f);
	m_shakeScale = 1.0f + decay * 0.3f;
}

void Reticle::DrawSprite()
{
	bool isDeleteMode = false;
	bool isGrabActive = false;
	bool isGrabFar = false;
	if (auto player = m_wpPlayer.lock())
	{
		isDeleteMode = player->HasDeleteTarget();
		isGrabActive = player->IsGrabActive();
		isGrabFar    = player->IsGrabTargetFar();
	}

	KdTexture* tex = &m_texNormal;
	Math::Color color = { 1.0f,1.0f,1.0f,1.0f };

	if (isDeleteMode)		tex = &m_texDelete;
	else if (isGrabActive)	tex = &m_texGrab;
	else if (isGrabFar)
	{
		tex = &m_texGrab;	// 狙ってはいるので緑だが、薄くして「まだ届かない」を表す
		color.w = 0.4f;
	}

	float baseSize = 64.0f * m_shakeScale;
	float centerAdj = -(baseSize - 64.0f) * 0.5f;	// 中心基準で拡縮させる補正

	KdShaderManager::Instance().m_spriteShader.DrawTex(
		tex,
		centerAdj + m_shakeOffset.x,
		centerAdj + m_shakeOffset.y,
		baseSize, baseSize,
		nullptr, &color);
}

void Reticle::StartShake()
{
	m_isShaking  = true;
	m_shakeFrame = 0;
}
