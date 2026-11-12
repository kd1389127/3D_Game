#include "WandPickup.h"
#include "../../../Character/Player/Player.h"
#include "../../../Collision/Collision.h"

namespace
{
	// 手持ちの杖の魔力0と同じ暗い青
	const Math::Vector3 kStoneDarkBlue(0.0f, 0.03f, 0.3f);
}

void WandPickup::Init(const Math::Vector3& pos)
{
	m_basePos = pos;

	m_spModel = std::make_shared<KdModelWork>();
	m_spModel->SetModelData(kModelPath);
}

void WandPickup::Update()
{
	if (m_isPickedUp) { return; }

	// ゆっくり回りながら、上下にふわふわ浮かせる
	++m_frame;
	const float yaw = m_frame * kSpinSpeed;

	// ゆらゆらを0〜1に直す。0(一番低い時)が、台座の上に置いた位置になる
	const float bobRatio = (sinf(m_frame * kBobSpeed) + 1.0f) * 0.5f;
	const float bobY = bobRatio * kBobRange;

	m_mWorld = Math::Matrix::CreateScale(kModelScale)
		* Math::Matrix::CreateRotationY(yaw)
		* Math::Matrix::CreateTranslation(m_basePos + Math::Vector3(0.0f, bobY, 0.0f));

	// 左クリックを「押した瞬間」だけ反応させる(押しっぱなしで毎フレーム反応しないため)
	const bool isClick = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
	const bool isTriggered = isClick && !m_prevPressed;
	m_prevPressed = isClick;

	m_isAimed = false;	// 毎フレーム調べ直す

	const std::shared_ptr<Player> spPlayer = m_wpPlayer.lock();
	if (!spPlayer) { return; }

	// 見ている向き(Playerの行列の+Z。上下の角度も入っている)
	Math::Vector3 lookDir = Math::Vector3::TransformNormal(Math::Vector3(0.0f, 0.0f, 1.0f), spPlayer->GetMatrix());
	lookDir.Normalize();

	// 杖を囲む箱(根元から先端まで)に視線が当たったら「狙っている」
	const Math::Vector3 boxCenter = m_mWorld.Translation() + Math::Vector3(0.0f, kWandLength * 0.5f, 0.0f);
	const Math::Vector3 boxHalf(kAimRadius, kWandLength * 0.5f, kAimRadius);
	if (!IntersectRayAABB(spPlayer->GetPos(), lookDir, boxCenter, boxHalf, kPickupRange)) { return; }

	m_isAimed = true;	// ここまで来たら「狙えている」(ヒントを出す)

	if (!isTriggered) { return; }

	m_isPickedUp = true;
	m_isAimed = false;
	if (m_onPickedUp) { m_onPickedUp(); }
}

void WandPickup::DrawLit()
{
	if (m_isPickedUp || !m_spModel) { return; }

	KdShaderManager::Instance().m_StandardShader.DrawModel(*m_spModel, m_mWorld, kWhiteColor, kStoneDarkBlue);
}