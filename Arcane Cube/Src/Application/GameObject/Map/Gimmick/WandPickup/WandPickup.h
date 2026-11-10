#pragma once

class Player;

// チュートリアルの台座の杖。レティクルを合わせてEキーか左クリックで取る(Portalの銃のイメージ)
class WandPickup : public KdGameObject
{
public:
	WandPickup() {}
	~WandPickup() override {}

	void Init(const Math::Vector3& pos);
	void Update() override;
	void DrawLit() override;

	void SetTarget(const std::shared_ptr<Player>& player) { m_wpPlayer = player; }

	// 取った瞬間に1回呼ばれる処理(杖と魔力ゲージを出す処理を、GameSceneから渡す)
	void SetOnPickedUp(const std::function<void()>& onPickedUp) { m_onPickedUp = onPickedUp; }

	bool IsPickedUp() const { return m_isPickedUp; }

private:
	static constexpr const char* kModelPath = "Asset/Models/Weapon/NewMagicwand/NewMagicwandBlue.gltf";
	static constexpr float kModelScale		= 16.0f;	
	static constexpr float kSpinSpeed		= 0.02f;	// 回る速さ(ラジアン/フレーム)
	static constexpr float kBobSpeed		= 0.05f;	// 上下に揺れる速さ
	static constexpr float kBobRange		= 3.0f;		// 浮き上がる幅(一番低い位置から一番高い位置までの差)
	static constexpr float kPickupRange		= 40.0f;	// 取れる距離
	static constexpr float kAimRadius		= 8.0f;		// 視線からこの距離以内なら「狙っている」(広めで親切に)

	std::shared_ptr<KdModelWork> m_spModel = nullptr;
	std::weak_ptr<Player> m_wpPlayer;
	std::function<void()> m_onPickedUp;

	Math::Vector3 m_basePos = Math::Vector3::Zero;	// 揺れの中心
	int  m_frame = 0;
	bool m_prevPressed = false;						// 前フレームにEか左クリックが押されていたか
	bool m_isPickedUp = false;
};