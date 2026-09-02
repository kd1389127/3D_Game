#pragma once
#include "../WeaponBase.h"

class NormalBlock; 
class Ground;

class Magicwand : public WeaponBase
{
public:

	Magicwand() {}
	~Magicwand() override {}

	void Init()		override;
	void Update()	override;

	void ShotBullet(const bool _rayFlg = false) override;

private:

	bool m_mouseDownFlg = false;       // 左クリックの「押した瞬間」だけ反応させるためのフラグ
	bool m_rightMouseDownFlg = false;  // 右クリック(キャンセル操作)用の同様のフラグ


	// ===================================================
	// せり出し調整モード関連
	// 「弾が着弾する」→「段数を調整する(Adjusting)」→「確定して生成する」
	// という2段階の操作フローを管理するための状態
	// ===================================================
	enum class WandState
	{
		Idle,       // 通常状態：左クリックで弾を発射できる
		Adjusting,  // 着弾後、段数・方向を確認しながら調整している状態
	};
	WandState m_state = WandState::Idle;

	Math::Vector3 m_aimBaseCell = Math::Vector3::Zero; // せり出しの起点になるマス(着弾点に一番近いグリッド座標)
	Math::Vector3 m_aimDir = Math::Vector3::Up;         // せり出していく方向(狙った面の法線。床なら上、壁なら横)
	int m_stackCount = 1;                               // 現在ホイールで選択している「何段せり出すか」の数
	static constexpr int m_maxStackCount = 5;           // 一度に生成できる最大段数


	// 調整中に表示している半透明プレビュー用ブロックのリスト
	// (実体はまだ確定していない「見本」なので、確定/キャンセル時にはExpireで消す)
	std::vector<std::shared_ptr<NormalBlock>> m_previewBlocks;

	// 弾が着弾した時にBulletから呼ばれる：調整モードに入る
	void EnterAdjustMode(const Math::Vector3& hitPos, const Math::Vector3& axisNormal);

	// 現在のシーンからGroundインスタンスを探して壁コライダーを取得する
	std::shared_ptr<Ground> FindGround() const;

	// 調整モード中、毎フレームの入力処理(ホイール/クリック)をまとめて行う
	void UpdateAdjustMode();

	// 現在のm_stackCount・m_aimDirを元に、プレビューブロックを作り直す
	void RebuildPreview();

	// 確定操作：プレビューを消し、実際にせり出しアニメーション付きでブロックを生成する
	void ConfirmStack();

	// キャンセル操作：何も生成せず、プレビューだけ消してIdleに戻る
	void CancelAdjustMode();
};