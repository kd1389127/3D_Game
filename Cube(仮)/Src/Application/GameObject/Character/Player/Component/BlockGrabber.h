#pragma once
#include <wrl/client.h>

class NormalBlock;
class BlockGridManager;
class IGrabbable;

class BlockGrabber
{
public:
	BlockGrabber() = default;
	~BlockGrabber() = default;

	void Init(float groundHeight = 0.0f);

	// PlayerのUpdateから呼ばれる
	void Update(const Math::Vector3& playerPos, const Math::Matrix& playerRotMat);

	// 現在ブロックを持っているか？
	bool IsCarrying() const { return m_spCarriedBlock != nullptr; }

	float GetMinY() const;   // ← 中身は書かず、宣言だけにする

	// 現在持っているブロックの座標を取得（持っていなければ判定不要なので呼び出し側でIsCarrying()チェック）
	Math::Vector3 GetCarriedBlockPos() const;
	
	void DrawPreview();

private:
	void HandleGrabAndDrop(const Math::Vector3& playerPos, const Math::Matrix& playerRotMat);
	void UpdateCarriedPos(const Math::Vector3& playerPos, const Math::Matrix& playerRotMat);
	void HandleDistanceControl(); // ★ ホイール入力で距離を変更する処理

	Math::Vector3 CalcTargetPos(const Math::Vector3& playerPos, const Math::Matrix& playerRotMat) const;

	// 設置プレビュー用
	Math::Vector3 m_previewPos = Math::Vector3::Zero;
	bool m_previewValid = false; // その位置に置けるか(占有されていないか)
	std::shared_ptr<KdModelWork> m_spNormalPreviewModel;  // 通常ブロック用プレビュー
	std::shared_ptr<KdModelWork> m_spGimmickPreviewModel; // ギミックブロック用プレビュー
	std::shared_ptr<KdModelWork> m_spActivePreviewModel;  // 今表示すべきプレビュー(どちらかを指す)
	Microsoft::WRL::ComPtr<ID3D11BlendState> m_alphaBlendState;

	std::shared_ptr<KdGameObject> m_spCarriedBlock = nullptr;

	bool m_eKeyFlg = false;

	// ブロックとの保持距離（初期値 30.0f）
	float m_holdDistance = 30.0f;

	float m_groundHeight = 0.0f;
};