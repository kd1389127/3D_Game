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

	// 右クリック長押し中(Aiming状態)かどうかを外部(FPSCameraなど)から確認するための関数
	bool IsAiming() const { return m_state == WandState::Aiming; }

private:

	// ===================================================
	// 長押しエイム(狙い)モード関連
	// 「右クリック長押し→視線の先にプレビュー表示→離すと位置固定→
	//  ホイールで段数調整→左クリックで確定発射 / 右クリックでキャンセル」
	// という操作フローを管理する状態
	// ===================================================
	enum class WandState
	{
		Idle,		// 通常状態：右クリックでエイムモードに入る
		Aiming,		// 長押し中：レイを飛ばし続けてプレビューを追従させている状態
		Adjusting,	// 離した後：プレビュー位置が固定、ホイールで調整状態
	};
	WandState m_state = WandState::Idle;

	// ----- ボタンの「押した/離した瞬間」を検知するための前フレーム状態 -----
	bool m_rightDownPrev = false;
	bool m_leftDownPrev  = false;

	// ----- エイム中(狙っている最中)の情報 -----
	Math::Vector3 m_aimBaseCell = Math::Vector3::Zero;  // せり出しの起点になるマス(着弾点に一番近いグリッド座標)
	Math::Vector3 m_aimDir = Math::Vector3::Up;         // せり出していく方向(狙った面の法線。床なら上、壁なら横)
	int m_stackCount = 1;                               // 現在ホイールで選択している「何段せり出すか」の数
	static constexpr int m_maxStackCount = 5;           // 一度に生成できる最大段数
	bool m_hasValidAim = false;							// 現在何かに向かって狙えているか(何も無い方向を向いた時の保険)


	// 調整中に表示している半透明プレビュー用ブロックのリスト
	// (実体はまだ確定していない「見本」なので、確定/キャンセル時にはExpireで消す)
	std::vector<std::shared_ptr<NormalBlock>> m_previewBlocks;

	// Idle状態での左クリック：エイムなしで即座に1個だけブロックを生成する(簡易射撃)
	void SingleShot(const Math::Vector3& muzzlePos, const Math::Matrix& parentMat);

	// 右クリックを押した瞬間に呼ばれる：エイムモードに入る
	void EnterAimMode();

	// 現在のシーンからGroundインスタンスを探して壁コライダーを取得する
	std::shared_ptr<Ground> FindGround() const;

	// Aiming中、毎フレーム呼ばれる：レイキャストで狙いを更新し、
	// (ボタン判定は行わない。Update側で状態遷移を管理する)
	void UpdateAimMode(const Math::Vector3& muzzlePos, const Math::Matrix& parentMat);

	// Adjusting中、毎フレーム呼ばれる：レティクルがどのプレビューブロックに合っているか判定する
	void UpdateHighlight(const Math::Vector3& muzzlePos, const Math::Matrix& parentMat);

	// 現在の狙い情報を元に、プレビューブロックを作り直す
	void RebuildPreview();

	// 確定操作：狙いの情報をコピーして弾に持たせ、発射する(この時点ではまだ生成しない)
	void ConfirmStack(const Math::Vector3& muzzlePos, const Math::Matrix& parentMat);

	// 弾が着弾した瞬間に呼ばれる：実際にせり出しアニメーション付きでブロックを生成する
	void GenerateStackAt(const Math::Vector3& baseCell, const Math::Vector3& dir, int stackCount);

	// キャンセル操作：何も生成せず、プレビューだけ消してIdleに戻る
	void CancelAim();

	// ----- 生成したスタックの履歴(古いスタックから消すため) -----
	std::deque<std::vector<std::weak_ptr<NormalBlock>>> m_generatedStacks;
	static constexpr int m_maxAliveStacks = 5; // 同時に存在できるスタックの上限

	// 指定したスタックを、先端(伸ばした側)から根元へ向かって消していく
	void DismissStack(const std::vector<std::weak_ptr<NormalBlock>>& stack);

};