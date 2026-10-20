#include "ExitLight.h"

void ExitLight::Init(const Math::Vector3& pos)
{
	// リザルトの背景画像の一部(夕日と湖のあたり)を、出口の光として貼る
	m_spPolygon = std::make_shared<KdSquarePolygon>("Asset/Textures/Result/Result.png");
	m_spPolygon->SetScale(Math::Vector2(kWidth, kHeight));

	// 画像の中の使う範囲(左上と右下。0〜1の割合)。見ながら調整する
	m_spPolygon->SetUVRect(Math::Vector2(0.45f, 0.15f), Math::Vector2(0.89f, 0.95f));

	// 板ポリゴンは+Z向きに作られるので、階段を上る向き(-X方向)から見て
	// 左右が逆にならないように、Y軸を-90度回してから置く
	m_mWorld = Math::Matrix::CreateRotationY(DirectX::XMConvertToRadians(-90.0f))
		* Math::Matrix::CreateTranslation(pos);
}

void ExitLight::DrawUnLit()
{
	if (!m_spPolygon) return;

	auto& shader = KdShaderManager::Instance().m_StandardShader;

	// 霧で暗くならないようにする(描画後に、自動で元に戻る)
	shader.SetFogEnable(false);

	shader.DrawPolygon(*m_spPolygon, m_mWorld,
		Math::Color(kBrightness, kBrightness, kBrightness, 1.0f));
}