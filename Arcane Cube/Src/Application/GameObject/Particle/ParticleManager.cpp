#include "ParticleManager.h"

#include <algorithm>

float ParticleManager::Rand(float _min, float _max)
{
	return _min + (_max - _min) * (rand() / (float)RAND_MAX);
}

void ParticleManager::MakeTangents(const Math::Vector3& _n, Math::Vector3& _t1, Math::Vector3& _t2)
{
	// 法線とほぼ平行でない軸を選び、外積で垂直な2軸を作る
	Math::Vector3 axis = (fabsf(_n.y) < 0.9f) ? Math::Vector3(0, 1, 0) : Math::Vector3(1, 0, 0);
	_t1 = _n.Cross(axis);
	_t1.Normalize();
	_t2 = _n.Cross(_t1);
}

// ---- 弾かれた:魔力が壁に拒否されて散る ----
void ParticleManager::EmitReflect(const Math::Vector3& _pos, const Math::Vector3& _normal)
{
	Math::Vector3 t1, t2;
	MakeTangents(_normal, t1, t2);

	for (int i = 0; i < 18; i++)
	{
		Particle p;
		p.pos = _pos + _normal * 0.5f;		// 壁にめり込まないよう少し浮かせる

		// 壁の面に沿って四方へ + 法線方向(壁から離れる向き)に少し
		float ang = Rand(0.0f, DirectX::XM_2PI);
		Math::Vector3 dir = t1 * cosf(ang) + t2 * sinf(ang);
		p.vel = dir * Rand(0.15f, 0.5f) + _normal * Rand(0.1f, 0.3f);

		p.gravity = -0.01f;
		p.drag = 0.93f;

		// 2割は白い芯、残りは弾の色(青緑系。弾の色に合わせて調整)
		if (i % 5 == 0) p.color = { 1.0f, 1.0f, 1.0f, 1.0f };
		else            p.color = { 0.3f, 1.0f, 0.8f, 1.0f };

		p.size = Rand(0.4f, 0.9f);
		p.maxLife = p.life = (int)Rand(18, 30);
		m_particles.push_back(p);
	}
}

// ---- 持ち上げ:緑の粒がふわっと散る ----
void ParticleManager::EmitGrab(const Math::Vector3& _blockCenter)
{
	for (int i = 0; i < 16; i++)
	{
		Particle p;
		p.pos = _blockCenter + Math::Vector3(Rand(-4, 4), Rand(-4, 4), Rand(-4, 4));

		// 中心から外向き + 少し上
		Math::Vector3 dir = p.pos - _blockCenter;
		if (dir.LengthSquared() > 0.0001f) dir.Normalize();
		p.vel = dir * Rand(0.1f, 0.3f) + Math::Vector3(0, 0.1f, 0);

		p.gravity = 0.0f;
		p.drag = 0.95f;
		p.color = { 0.3f, 1.0f, 0.4f, 1.0f };	// レティクルの緑に寄せる
		p.size = Rand(0.4f, 0.8f);
		p.maxLife = p.life = (int)Rand(20, 30);
		m_particles.push_back(p);
	}
}

// ---- ゴール:粒が昇り続ける ----
void ParticleManager::EmitGoal(const Math::Vector3& _goalPos)
{
	for (int i = 0; i < 2; i++)
	{
		Particle p;
		float ang = Rand(0.0f, DirectX::XM_2PI);
		float r = Rand(1.0f, 4.0f);
		p.pos = _goalPos + Math::Vector3(cosf(ang) * r, Rand(-3.0f, 0.0f), sinf(ang) * r);

		p.vel = Math::Vector3(0, Rand(0.08f, 0.2f), 0);
		p.color = { 1.0f, 0.9f, 0.5f, 1.0f };	// 金色寄り(ゴールの見た目に合わせて調整)
		p.size = Rand(0.4f, 0.9f);
		p.maxLife = p.life = (int)Rand(40, 60);
		m_particles.push_back(p);
	}
}

// ---- 毎フレームの更新 ----
void ParticleManager::Update()
{
	for (auto& p : m_particles)
	{
		p.vel.y += p.gravity;
		p.vel *= p.drag;
		p.pos += p.vel;
		p.life--;
	}

	// 寿命が尽きた粒を消す
	m_particles.erase(
		std::remove_if(m_particles.begin(), m_particles.end(),
			[](const Particle& p) { return p.life <= 0; }),
		m_particles.end());
}

// ---- 描画 ----
void ParticleManager::Draw()
{
	if (m_particles.empty()) return;

	// 板ポリは最初に描くときだけ作る
	if (!m_poly)
	{
		m_poly = std::make_shared<KdSquarePolygon>("Asset/Textures/Particle/particle.png");
	}

	// 粒をカメラの方へ向ける(ビルボード):カメラの回転だけを取り出す
	// ※GetCameraCB().mView はフレームワークに合わせて要確認
	Math::Matrix camRot = KdShaderManager::Instance().GetCameraCB().mView.Invert();
	camRot.Translation(Math::Vector3::Zero);

	// 粒の大きさの倍率(見た目が小さすぎる・大きすぎる時はここを調整)
	const float kDrawScale = 2.0f;

	for (auto& p : m_particles)
	{
		// 寿命が尽きるほど透明にする
		float rate = (float)p.life / (float)p.maxLife;
		Math::Color col = p.color;
		col.A(col.A() * rate);

		Math::Matrix mat = Math::Matrix::CreateScale(p.size * kDrawScale)
			* camRot
			* Math::Matrix::CreateTranslation(p.pos);

		KdShaderManager::Instance().m_StandardShader.DrawPolygon(*m_poly, mat, col);
	}
}

// ---- ParticleDrawer ----
void ParticleDrawer::Update()
{
	ParticleManager::Instance().Update();
}

void ParticleDrawer::DrawEffect()
{
	ParticleManager::Instance().Draw();
}