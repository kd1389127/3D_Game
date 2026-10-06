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
	void DrawUnLit()	override;

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

	// 消滅アニメーションを開始する(StartEmergeの逆再生)
	// delayFrames … 何フレーム後に消滅を開始するか(スタックの先端から順に消すための時間差)
	void StartDismiss(int delayFrames);

	void SetHighlight(bool isHighlighted) { m_isHighlighted = isHighlighted; }

	// 面だけをハイライトする(true=表示, localNormalはブロックのローカル空間での面法線)
	void SetHighlightFace(bool enable, const Math::Vector3& localNormal = Math::Vector3::Up)
	{
		m_isFaceHighlighted = enable;
		if (enable) m_highlightFaceNormal = localNormal;
	}

	// 削除モード中、レティクルが合っているブロックを赤くハイライトする
	void SetDeleteHighlight(bool enable) { m_isDeleteTargeted = enable; }
	bool IsDeleteHighlighted()const { return m_isDeleteTargeted; }

	// このブロックがどの「スタック(1回の発動でまとめて生成された一連のブロック)」に属するか
	// stackIndexは0=基点、数字が大きいほど先端側(消す時の演出の順番に使う)
	void SetStackInfo(int stackId, int stackIndex) { m_stackId = stackId; m_stackIndex = stackIndex; }
	int GetStackId() const { return m_stackId; }
	int GetStackIndex() const { return m_stackIndex; }

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

	float m_dissolveProgress = 1.0f;				  // 0=出現前、1=完全に実体化済み(通常時は1で固定)


	// ----- 消滅アニメーション用のパラメータ(StartEmergeと対になる仕組み) -----
	bool m_isDismissing = false;
	int  m_dismissFrame = 0;
	static constexpr int m_dismissDuration = 24;	  // 消滅にかかるフレーム数

	// 選択中のハイライト
	bool m_isHighlighted = false;	// レティクルが合っている間true
	int  m_blinkTimer	 = 0;

	// 選択中の面のハイライト
	bool m_isFaceHighlighted = false;
	Math::Vector3 m_highlightFaceNormal = Math::Vector3::Up;

	// 面ハイライトの描画本体
	void DrawFaceHighlight();

	// 削除モード中、レティクルがあっているか
	bool m_isDeleteTargeted = false;

	int m_stackId = -1;   // 所属するスタックのID(-1=未所属/単体)
	int m_stackIndex = 0; // スタック内の順番(先端から消すアニメの遅延計算に使う)
};