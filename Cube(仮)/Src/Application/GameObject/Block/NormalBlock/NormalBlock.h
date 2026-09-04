#pragma once

#include "../Grabbable.h"

class NormalBlock : public KdGameObject, public IGrabbable
{
public:

	NormalBlock() {}
	~NormalBlock()		override {}

	void Init(const Math::Vector3& pos);
	void Update()	override;      // ★せり出しアニメーションの進行をここで処理する
	void PostUpdate() override;
	void DrawLit()	override;

	void SetCarried(bool isCarried);
	bool IsCarried() const { return m_isCarried; }
	BlockGridManager::BlockKind GetBlockKind() const override
	{
		return BlockGridManager::BlockKind::Normal;
	}

	// ===================================================
	// せり出し演出関連
	// ===================================================

	// せり出しアニメーションを開始する
	// delayFrames … 何フレーム後にアニメーションを開始するか(複数段を時間差で出現させるため)
	// dir         … どの方向からせり出してくるか(=最終位置からdir方向に1マス分下がった位置がスタート地点)
	void StartEmerge(int delayFrames, const Math::Vector3& dir = Math::Vector3::Up);

	// このブロックを即座に消滅させる(主にプレビュー用ブロックの後片付けに使う)
	void Expire() { m_isExpired = true; }

	// プレビュー(見本)状態を切り替える。プレビュー中は当たり判定を無効化する
	void SetPreview(bool isPreview);
	bool IsPreview() const { return m_isPreview; }

private:

	std::shared_ptr<KdModelWork> m_spModel = nullptr;

	bool m_isCarried = false; // プレイヤーに持ち上げられている最中かどうか
	bool m_isPreview = false; // プレビュー(まだ確定していない見本)かどうか

	// ----- せり出しアニメーション用のパラメータ -----
	bool  m_isEmerging = false;                       // アニメーション再生中かどうか
	int   m_emergeFrame = 0;                          // 経過フレーム数(負の値の間はまだ開始待ち＝delay中)
	static constexpr int m_emergeDuration = 24;       // アニメーションが完了するまでのフレーム数
	Math::Vector3 m_finalPos = Math::Vector3::Zero;   // 最終的に静止する座標(グリッドにスナップ済み)
	Math::Vector3 m_emergeDir = Math::Vector3::Up;    // せり出してくる方向(スタート地点の計算に使う)

	float m_dissolveProgress = 1.0f; // 0=出現前、1=完全に実体化済み(通常時は1で固定)
};