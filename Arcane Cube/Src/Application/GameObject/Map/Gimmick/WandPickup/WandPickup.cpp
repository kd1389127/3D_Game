#include "WandPickup.h"
#include "../../../Character/Player/Player.h"

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

	// Eキーか左クリックを「押した瞬間」だけ反応させる(押しっぱなしで毎フレーム反応しないため)
	const bool isKeyE = (GetAsyncKeyState('E') & 0x8000) != 0;
	const bool isClick = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
	const bool isPressed = isKeyE || isClick;
	const bool isTriggered = isPressed && !m_prevPressed;
	m_prevPressed = isPressed;
	if (!isTriggered) { return; }

	const std::shared_ptr<Player> spPlayer = m_wpPlayer.lock();
	if (!spPlayer) { return; }

	// 見ている向き(Playerの行列の+Z。上下の角度も入っている)
	Math::Vector3 lookDir = Math::Vector3::TransformNormal(Math::Vector3(0.0f, 0.0f, 1.0f), spPlayer->GetMatrix());
	lookDir.Normalize();

	// 目から杖までのベクトルを、「視線に沿った成分」と「視線からのずれ」に分けて考える
	const Math::Vector3 toWand = m_mWorld.Translation() - spPlayer->GetPos();
	const float along = toWand.Dot(lookDir);
	if (along < 0.0f || along > kPickupRange) { return; }	// 後ろ・遠すぎは取れない

	const Math::Vector3 offset = toWand - lookDir * along;
	if (offset.Length() > kAimRadius) { return; }			// レティクルが杖から外れている

	m_isPickedUp = true;
	if (m_onPickedUp) { m_onPickedUp(); }
}

void WandPickup::DrawLit()
{
	if (m_isPickedUp || !m_spModel) { return; }

	KdShaderManager::Instance().m_StandardShader.DrawModel(*m_spModel, m_mWorld, kWhiteColor, kStoneDarkBlue);
}