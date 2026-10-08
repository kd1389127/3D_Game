#pragma once

class KdSquarePolygon;

// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
// パーティクル管理(シングルトン)
// 粒の発生・更新・描画をここでまとめて行う
// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
class ParticleManager
{
public:
	// 弾かれた:着弾点から外へ散る(1回)
	void EmitReflect(const Math::Vector3& _pos, const Math::Vector3& _normal);

	// 持ち上げ:ブロックの周りから緑の粒が散る(1回)
	void EmitGrab(const Math::Vector3& _blockCenter);

	// ゴール:呼ぶたびに粒が昇る(毎フレーム呼んでよい)
	void EmitGoal(const Math::Vector3& _goalPos);

	void Update();
	void Draw();

	// ステージ開始時に全部消す
	void Clear() { m_particles.clear(); }

private:
	struct Particle
	{
		Math::Vector3 pos;
		Math::Vector3 vel;			// 1フレームの移動量
		float gravity = 0.0f;		// 毎フレーム velのYに足す値
		float drag = 1.0f;			// 毎フレーム velに掛ける値(1.0で減速なし)
		Math::Color color = { 1, 1, 1, 1 };
		float size = 1.0f;
		int life = 0;				// 残りフレーム
		int maxLife = 1;			// 最初の寿命(透明度の計算用)
	};

	std::vector<Particle> m_particles;

	// 粒の描画に使う板ポリ(最初の描画時に作る)
	std::shared_ptr<KdSquarePolygon> m_poly = nullptr;

	// 範囲内のランダムな値
	static float Rand(float _min, float _max);

	// 法線に垂直な2本の軸(円状に散らすのに使う)
	static void MakeTangents(const Math::Vector3& _n, Math::Vector3& _t1, Math::Vector3& _t2);

private:
	ParticleManager() {}
	~ParticleManager() {}

public:
	static ParticleManager& Instance()
	{
		static ParticleManager instance;
		return instance;
	}
};

// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
// ParticleManagerのUpdate/Drawを、シーンのオブジェクトリスト経由で呼ばせるための仲介役
// GameScene::Initでオブジェクトリストに1つ追加する
// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
class ParticleDrawer : public KdGameObject
{
public:
	void Update() override;
	void DrawEffect() override;
};