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

private:

	bool m_mouseDownFlg = false;       // 左クリックの「押した瞬間」だけ反応させるためのフラグ
	//bool m_rightMouseDownFlg = false;  // 右クリック(キャンセル操作)用の同様のフラグ

	// ===================================================
	// 長押しエイム(狙い)モード関連
	// 「左クリック長押し→視線の先にプレビュー表示→ホイールで段数調整→
	//  離して確定 / 右クリックでキャンセル」という操作フローを管理する状態
	// ===================================================
	enum class WandState
	{
		Idle,    // 通常状態：左クリックでエイムモードに入る
		Aiming,  // 長押し中：レイを飛ばし続けてプレビューを追従させている状態
	};
	WandState m_state = WandState::Idle;

	Math::Vector3 m_aimBaseCell = Math::Vector3::Zero;  // せり出しの起点になるマス(着弾点に一番近いグリッド座標)
	Math::Vector3 m_aimDir = Math::Vector3::Up;         // せり出していく方向(狙った面の法線。床なら上、壁なら横)
	int m_stackCount = 1;                               // 現在ホイールで選択している「何段せり出すか」の数
	static constexpr int m_maxStackCount = 5;           // 一度に生成できる最大段数
	bool m_hasValidAim = false;							// 現在何かに向かって狙えているか(何も無い方向を向いた時の保険)


	// 調整中に表示している半透明プレビュー用ブロックのリスト
	// (実体はまだ確定していない「見本」なので、確定/キャンセル時にはExpireで消す)
	std::vector<std::shared_ptr<NormalBlock>> m_previewBlocks;

	// 左クリックを押した瞬間に呼ばれる：エイムモードに入る
	void EnterAimMode();

	// 現在のシーンからGroundインスタンスを探して壁コライダーを取得する
	std::shared_ptr<Ground> FindGround() const;

	// エイムモード中、毎フレームの入力処理(レイキャスト追従/ホイール/クリック)をまとめて行う
	void UpdateAimMode(const Math::Vector3& muzzlePos, const Math::Matrix& parentMat);

	// 現在のm_stackCount・m_aimDirを元に、プレビューブロックを作り直す
	void RebuildPreview();

	// 確定操作：狙いを確定し、その場所に向けて弾を発射する(生成は着弾時に行う)
	void ConfirmStack(const Math::Vector3& muzzlePos);

	// 弾が着弾した瞬間に呼ばれる：実際にせり出しアニメーション付きでブロックを生成する
	void GenerateStackAt(const Math::Vector3& baseCell, const Math::Vector3& dir, int stackCount);

	// キャンセル操作：何も生成せず、プレビューだけ消してIdleに戻る
	void CancelAim();
};