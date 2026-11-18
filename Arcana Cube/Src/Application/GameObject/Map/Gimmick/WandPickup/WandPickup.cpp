#include "WandPickup.h"
#include "../../../Character/Player/Player.h"
#include "../../../Collision/Collision.h"

namespace
{
	// 手持ちの杖の魔力0と同じ暗い青
	const Math::Vector3 kStoneDarkBlue(0.0f, 0.03f, 0.3f);

	// 手元の位置(Magicwand::Initで杖を置いている、プレイヤーから見た相対位置と同じ値)
	const Math::Vector3 kHandLocalPos(0.8f, -0.7f, 0.6f);
}

void WandPickup::Init(const Math::Vector3& pos)
{
	m_basePos = pos;

	m_spModel = std::make_shared<KdModelWork>();
	m_spModel->SetModelData(kModelPath);
}

void WandPickup::Update()
{
	// 取った後：手元へ飛んでいき、縮みながら吸い込まれる。着いたら杖と魔力ゲージを出す
	if (m_isPickedUp)
	{
		if (!m_isFlying) { return; }

		++m_frame;
		++m_flyFrame;
		
		const float progress = std::min(static_cast<float>(m_flyFrame) / kFlyFrames, 1.0f);

		// 位置：ゆっくり動き出して、ゆっくり着く(smoothstep)。動きが全体に散らばって見える
		const float moveEase = progress * progress * (3.0f - 2.0f * progress);
		// 大きさ：最初に素早く縮む(ease-out)。大きな杖が画面を横切らないようにする
		const float scaleEase = 1.0f - (1.0f - progress) * (1.0f - progress);

		const std::shared_ptr<Player> spPlayer = m_wpPlayer.lock();
		if (spPlayer)
		{
			// 手元は毎フレーム求め直す(飛んでいる間にプレイヤーが動いても追いかけるため)
			const Math::Vector3 handPos = Math::Vector3::Transform(kHandLocalPos, spPlayer->GetMatrix());
			const Math::Vector3 pos		= Math::Vector3::Lerp(m_flyStartPos, handPos, moveEase);
			
			const float scale = kModelScale + (kHandScale - kModelScale) * scaleEase;

			m_mWorld = Math::Matrix::CreateScale(scale)
				* Math::Matrix::CreateRotationY(m_frame * kSpinSpeed)
				* Math::Matrix::CreateTranslation(pos);
		}

		if (m_flyFrame >= kFlyFrames)
		{
			m_isFlying = false;
			if (m_onPickedUp) { m_onPickedUp(); }
		}
		return;
	}

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
	const bool isClick	   = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
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

	m_isPickedUp  = true;
	m_isAimed	  = false;
	m_isFlying	  = true;
	m_flyStartPos = m_mWorld.Translation();	// 今の浮いている位置から飛び始める

	auto se = KdAudioManager::Instance().Play(kPickupSePath, false);
	if (se) { se->SetVolume(kPickupSeVolume); }
}

void WandPickup::DrawLit()
{
	if (!m_spModel) { return; }
	if (m_isPickedUp && !m_isFlying) { return; }	// 手元に着いたら描かない(以降は手持ちの杖が出る)

	KdShaderManager::Instance().m_StandardShader.DrawModel(*m_spModel, m_mWorld, kWhiteColor, kStoneDarkBlue);
}