#pragma once

class Player;

// チュートリアルの台座の杖。レティクルを合わせて左クリックで取る
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

	// レティクルが杖に合っているか(ヒント表示に使う)
	bool IsAimed() const { return m_isAimed; }

private:
	static constexpr const char* kModelPath = "Asset/Models/Weapon/NewMagicwand/NewMagicwandBlue.gltf";
	static constexpr float kModelScale		= 16.0f;	
	static constexpr float kSpinSpeed		= 0.02f;	// 回る速さ(ラジアン/フレーム)
	static constexpr float kBobSpeed		= 0.05f;	// 上下に揺れる速さ
	static constexpr float kBobRange		= 3.0f;		// 浮き上がる幅(一番低い位置から一番高い位置までの差)
	static constexpr float kPickupRange		= 20.0f;	// 取れる距離
	static constexpr float kAimRadius		= 1.5f;		// 視線からこの距離以内なら「狙っている」(広めで親切に)
	static constexpr float kWandLength		= 10.0f;	// 杖の長さ(根元から先端まで)

	// 杖取得時の演出
	static constexpr const char* kPickupSePath = "Asset/Sounds/SE/Block/Generate.wav";	
	static constexpr float kPickupSeVolume	   = 0.1f;
	static constexpr int   kFlyFrames		   = 30;	// 手元まで飛ぶ長さ(60fpsで約0.4秒)
	static constexpr float kHandScale		   = 1.3f;	// 手持ちの杖と同じ大きさ(Magicwand::Initのスケール)

	std::shared_ptr<KdModelWork> m_spModel = nullptr;
	std::weak_ptr<Player> m_wpPlayer;
	std::function<void()> m_onPickedUp;

	Math::Vector3 m_basePos = Math::Vector3::Zero;		// 揺れの中心
	int  m_frame = 0;
	bool m_prevPressed = false;							// 前フレームに左クリックが押されていたか
	bool m_isPickedUp  = false;
	bool m_isAimed	   = false;							// 今、レティクルが杖に合っているか

	Math::Vector3 m_flyStartPos = Math::Vector3::Zero;	// 飛び始めの位置
	int  m_flyFrame = 0;
	bool m_isFlying = false;							// 取ってから手元に着くまでtrue

};