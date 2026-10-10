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

	// 弾の青を、壁の上でも見えるよう明るめに振った3色
	const Math::Color kMain = { 0.35f, 0.55f, 1.00f, 1.0f };	// 本体の青
	const Math::Color kCore = { 0.75f, 0.88f, 1.00f, 1.0f };	// ほぼ白に近い明るい芯
	const Math::Color kDeep = { 0.45f, 0.30f, 1.00f, 1.0f };	// 紫寄りの青
	
	// 着弾点から法線方向に浮かせた位置が、粒の出る起点
	// (階段のように判定板が見た目の内側にある場所でも、表面の外に出すため)
	const float kLift = 0.5f;
	const Math::Vector3 origin = _pos + _normal * kLift;

	// ---- 衝撃の閃光:その場で大きく光って一瞬で消える ----
	for (int i = 0; i < 3; ++i)
	{
		Particle p;
		p.pos = origin;
		p.vel = Math::Vector3::Zero;
		p.drag = 1.0f;
		p.color = (i == 0) ? kCore : kMain;
		p.size = (i == 0) ? 1.6f : 1.1f;		
		p.maxLife = p.life = 6;
		m_particles.push_back(p);
	}

	// ---- 衝撃波:壁の面に沿って放射状に走る光の筋 ----
	const int kRingNum = 24;
	for (int i = 0; i < kRingNum; ++i)
	{
		float ang = DirectX::XM_2PI * i * kRingNum;
		Math::Vector3 side = t1 * cosf(ang) + t2 * sinf(ang);

		Particle p;
		p.pos = origin;
		p.vel = side * 1.8f + _normal * 0.1f;
		p.drag = 0.86f;
		p.color = kMain;
		p.size = 0.35f;
		p.stretch = 5.0f;
		p.maxLife = p.life = 12;
		m_particles.push_back(p);
	}

	// ---- 火花:壁から跳ね返って飛び散る細い光 ----
	for (int i = 0; i < 40; ++i)
	{
		float ang = Rand(0.0f, DirectX::XM_2PI);
		Math::Vector3 side = t1 * cosf(ang) + t2 * sinf(ang);
		Math::Vector3 dir = _normal * Rand(0.4f, 1.0f) + side * Rand(0.3f, 1.0f);
		dir.Normalize();

		Particle p;
		p.pos = origin;
		p.vel = dir * Rand(1.0f, 3.0f);
		p.gravity = -0.03f;
		p.drag = 0.9f;

		if (i % 5 == 0)      p.color = kCore;
		else if (i % 3 == 0) p.color = kDeep;
		else                 p.color = kMain;

		p.size = Rand(0.25f, 0.55f);
		p.stretch = Rand(4.0f, 7.0f);
		p.maxLife = p.life = (int)Rand(14, 24);
		m_particles.push_back(p);
	}
	
}

// ---- 持ち上げ:緑の粒がふわっと散る ----
void ParticleManager::EmitGrab(const Math::Vector3& _blockCenter)
{
	for (int i = 0; i < 16; ++i)
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
	for (int i = 0; i < 2; ++i)
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

	// カメラの向き(ビルボード用)と、粒の動きをカメラから見た向きに直すための行列
	Math::Matrix view = KdShaderManager::Instance().GetCameraCB().mView;
	Math::Matrix camRot = view.Invert();
	camRot.Translation(Math::Vector3::Zero);

	// 粒の大きさの倍率(見た目が小さすぎる・大きすぎる時はここを調整)
	const float kDrawScale = 2.0f;

	for (auto& p : m_particles)
	{
		float rate = (float)p.life / (float)p.maxLife;
		Math::Color col = p.color;
		col.A(col.A() * rate);

		float sx = p.size * kDrawScale;	// 横(進行方向)の大きさ
		float sy = sx;					// 縦の大きさ
		Math::Matrix spin = Math::Matrix::Identity;

		// 引き伸ばす粒は、画面上で進む向きに細長く向ける
		if (p.stretch > 1.0f)
		{
			Math::Vector3 v = Math::Vector3::TransformNormal(p.vel, view);
			float speed = sqrtf(v.x * v.x + v.y * v.y);
			if (speed > 0.001f)
			{
				spin = Math::Matrix::CreateRotationZ(atan2f(v.y, v.x));
				sx *= p.stretch * (0.4f + speed * 0.5f);	// 速いほど長く、減速すると短くなる
				sy *= 0.6f;									// 細くする
			}
		}

		Math::Matrix mat = Math::Matrix::CreateScale(sx, sy, 1.0f)
			* spin
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