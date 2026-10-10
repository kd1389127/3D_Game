#include "WeaponBase.h"

namespace
{
	// 発光の強さ(1.0で、Blenderで設定した放射の色そのまま)
	const float kLitEmissive = 0.0f;		// 通常の描画:魔法石そのものの光り方
	const float kBrightEmissive = 2.5f;		// ブライト:石のまわりのにじみの強さ
	const float kFrameStep = 1.0f / 60.0f;
}

void WeaponBase::Update()
{
	// ----- 光のゆらぎ -----
	// 時間を進める(m_flickerSpeedが大きいほど速く進む=速く揺れる)
	m_glowTime += kFrameStep * m_flickerSpeed;

	// 周期の違う3つのsin波を足して、不規則な揺れを作る
	// 0.75f					… 揺れの中心
	// 0.12f / 0.08f / 0.05f    … 各波の振幅(大きい波ほど全体の動きを決める)
	// 2.0f / 5.3f / 11.7f      … 速さ(半端な数にして、波がそろわないようにする)
	// +1.2f / +2.6f            … 波の開始位置のずらし
	// 結果 約0.5〜1.0の範囲を動く
	m_flicker = 0.75f
		+ 0.12f * sinf(m_glowTime * 2.0f)			// ゆっくりした大きな波
		+ 0.08f * sinf(m_glowTime * 5.3f + 1.2f)	// 中くらいの波
		+ 0.05f * sinf(m_glowTime * 11.7f + 2.6f);	// 細かい揺れ

	const std::shared_ptr<const KdGameObject> spParent = m_wpParent.lock();
	if (spParent)
	{
		Math::Matrix parentMat = spParent->GetMatrix();
		m_mWorld = m_scaleMat * m_animMat * m_localMat * parentMat;
	}
}

void WeaponBase::DrawLit()
{
	if (!m_spModel) return;

	// ゆらぎを0〜1に直す(0=いちばん暗い瞬間、1=いちばん明るい瞬間)
	const float n = std::clamp((m_flicker - 0.5f) / 0.5f, 0.0f, 1.0f);

	// 発光 = 基準の強さ × 魔力の残りによる明るさ × ゆらぎ(0.85〜1.0倍と控えめ)
	// 石そのものは少しだけ揺らす(大きく揺らすと白く飽和して揺れが見えなくなる)
	const Math::Vector3 emissive = Math::Vector3::One * (kLitEmissive * m_emissiveLevel * (0.85f + 0.15f * n));

	KdShaderManager::Instance().m_StandardShader.DrawModel(*m_spModel, m_mWorld, kWhiteColor, emissive);
}


void WeaponBase::DrawBright()
{
	if (!m_spModel) return;

	// 魔力が0で消灯しているときは、にじみを描かない
	if (m_emissiveLevel <= 0.001f) return;

	// ゆらぎを0〜1に直す(DrawLitと同じ計算)
	const float n = std::clamp((m_flicker - 0.5f) / 0.5f, 0.0f, 1.0f);

	// にじみの強さ = 基準の強さ × 魔力の残りによる明るさ × ゆらぎ
	// (0.2f + 1.6f * n)は、nが0〜1なので0.2倍〜1.8倍。にじみを大きく呼吸させている
	// 強いほどにじみが広がり、色も青→紫に寄って見える	
	const Math::Vector3 emissive = Math::Vector3::One * (kBrightEmissive * m_emissiveLevel * (0.2f + 1.6f * n));

	// 発光する部分(魔法石)だけを描く。木の部分は発光が0なので黒になり、光らない
	KdShaderManager::Instance().m_StandardShader.SetOnlyEmissive(true);
	KdShaderManager::Instance().m_StandardShader.DrawModel(*m_spModel, m_mWorld, kWhiteColor, emissive);
}
