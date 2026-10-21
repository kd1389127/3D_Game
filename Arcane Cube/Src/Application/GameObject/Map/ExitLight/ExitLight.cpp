#include "ExitLight.h"
#include "../../../Scene/SceneManager.h"

void ExitLight::Init(const Math::Vector3& pos)
{
	m_spPolygon = std::make_shared<KdSquarePolygon>("Asset/Textures/Result/ExitGlow.png");
	m_spPolygon->SetScale(Math::Vector2(kGlowW, kGlowH));

	// 階段を上る-X方向から見える向き
	m_mWorld = Math::Matrix::CreateRotationY(DirectX::XMConvertToRadians(-90.0f))
		* Math::Matrix::CreateTranslation(pos);

	m_drawType = eDrawTypeUnLit | eDrawTypeBright | eDrawTypeUI;
}

void ExitLight::Update()
{
	if (m_triggered) { return; }	// 向かい始めたら、真っ白のまま固定する

	const std::shared_ptr<KdGameObject> spTarget = m_wpTarget.lock();
	if (!spTarget) { return; }

	// プレイヤーのx座標から、階段をどこまで上ったかを0〜1にする
	const float t = (spTarget->GetPos().x - kStairStartX) / (kFullWhiteX - kStairStartX);
	m_rate = std::clamp(t, 0.0f, 1.0f);

	if (m_rate >= 1.0f)
	{
		m_triggered = true;

		// 白いフェードでリザルトへ(画面はもう真っ白なので、白からつながる)
		SceneManager::Instance().RequestChangeScene(SceneManager::SceneType::Result, 0, true);
	}
}

void ExitLight::DrawSprite()
{
	if (m_rate <= 0.0f) { return; }

	// 最初はゆっくり、最後に一気に強くなる(InSine。KdEasingに頼らず自分で計算)
	const float e = 1.0f - cosf(m_rate * DirectX::XM_PIDIV2);

	auto& sp = KdShaderManager::Instance().m_spriteShader;
	Math::Color white = { 1.0f, 1.0f, 1.0f, e };
	sp.DrawBox(0, 0, 800, 450, &white, true);	// 画面全体を覆う白い四角
}

void ExitLight::DrawUnLit()
{
	if (!m_spPolygon) return;

	auto& shader = KdShaderManager::Instance().m_StandardShader;
	shader.SetFogEnable(false);		// 霧で暗くならないようにする

	// 少し暖かい白(朝日の色)
	shader.DrawPolygon(*m_spPolygon, m_mWorld,
		Math::Color(kGlowBright, kGlowBright * 0.97f, kGlowBright * 0.88f, 1.0f));
}

void ExitLight::DrawBright()
{
	if (!m_spPolygon) return;

	auto& shader = KdShaderManager::Instance().m_StandardShader;
	shader.SetFogEnable(false);		// 霧で暗くならないようにする

	// 少し暖かい白(朝日の色)
	shader.DrawPolygon(*m_spPolygon, m_mWorld,
		Math::Color(kGlowBright, kGlowBright * 0.97f, kGlowBright * 0.88f, 1.0f));
}
