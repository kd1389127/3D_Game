#include "Lift.h"

namespace
{
	// このゲームは通常ブロック(1mのモデル)を8倍している。リフトも同じ縮尺にそろえる
	constexpr float kModelScale = 8.0f;
}

void Lift::Init(const Math::Vector3& startPos, const Math::Vector3& endPos, float seconds)
{
	m_startPos = startPos;
	m_endPos = endPos;
	m_curPos = startPos;

	// 固定フレーム前提：60フレームで1秒なので、1フレームの進み量は 1 ÷ (60 × 秒)
	m_speed = 1.0f / (60.0f * seconds);

	if (!m_spModel)
	{
		m_spModel = std::make_shared<KdModelWork>();
		m_spModel->SetModelData("Asset/Models/Map/Lift/Lift.gltf");
	}

	// 当たり判定(モデルができた後に作る)
	if (!m_pCollider)
	{
		m_pCollider = std::make_unique<KdCollider>();	// ← 先にKdColliderを作る
		m_pCollider->RegisterCollisionShape("Lift", m_spModel,
			KdCollider::TypeGround | KdCollider::TypeBump);	// ← その後に登録
	}

	// ---- 上面の高さを測る ----
	// いったんモデルの原点を始点に置く
	m_mWorld = Math::Matrix::CreateScale(kModelScale) * Math::Matrix::CreateTranslation(m_startPos);

	// リフトの真上から真下へレイを飛ばし、最初に当たる面(＝上面)の高さを調べる
	KdCollider::RayInfo ray;
	ray.m_pos = m_startPos + Math::Vector3(0.0f, 200.0f, 0.0f);
	ray.m_dir = Math::Vector3::Down;
	ray.m_range = 400.0f;
	ray.m_type = KdCollider::TypeGround;

	std::list<KdCollider::CollisionResult> results;
	Intersects(ray, &results);

	// 当たった結果のうち、一番高い点を上面とする
	float topY = -FLT_MAX;
	for (auto& ret : results)
	{
		const Math::Vector3 hitPos = ret.m_hitPos;
		topY = std::max(topY, hitPos.y);
	}

	// 当たらなかった時は0のままにして、位置を変えない
	m_topOffset = (topY > -FLT_MAX) ? (topY - m_startPos.y) : 0.0f;
	KdDebugGUI::Instance().AddLog("Lift top offset = %.2f\n", m_topOffset);

	// 上面がstartPos.yにそろうよう、モデルをその分だけ下げて置き直す
	m_mWorld = Math::Matrix::CreateScale(kModelScale) *
		Math::Matrix::CreateTranslation(m_curPos - Math::Vector3(0.0f, m_topOffset, 0.0f));

	m_delta = Math::Vector3::Zero;
}

void Lift::Update()
{
	const Math::Vector3 prevPos = m_curPos;

	// 進捗度を -0.2 ～ 1.2 の間で往復させる
	m_progress += m_isReverse ? -m_speed : m_speed;
	if (m_progress >= 1.2f) { m_isReverse = true; }
	else if (m_progress <= -0.2f) { m_isReverse = false; }

	// 位置には、0～1に切り詰めた値を使う(両端で少し止まる)
	const float t = std::clamp(m_progress, 0.0f, 1.0f);

	// イージング(EaseInOutSineと同じ式)
	const float eased = 0.5f - 0.5f * cosf(DirectX::XM_PI * t);

	m_curPos = m_startPos + (m_endPos - m_startPos) * eased;

	// モデルは、上面がm_curPosにそろうよう、m_topOffsetだけ下げて置く
	m_mWorld = Math::Matrix::CreateScale(kModelScale) *
		Math::Matrix::CreateTranslation(m_curPos - Math::Vector3(0.0f, m_topOffset, 0.0f));

	// 今フレームに動いた量(Playerが読む)
	m_delta = m_curPos - prevPos;
}

void Lift::DrawLit()
{
	if (!m_spModel) return;

	KdShaderManager::Instance().m_StandardShader.DrawModel(*m_spModel, m_mWorld);
}