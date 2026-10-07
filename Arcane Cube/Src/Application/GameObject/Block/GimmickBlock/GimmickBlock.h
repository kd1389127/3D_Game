#pragma once

#include "../Grabbable.h"

// スイッチ起動用の専用ブロック
// 通常ブロック(NormalBlock)と同様にグリッドに設置・持ち運びされるが、
// BlockGridManager上では種別(GimmickKey)として区別して登録される
class GimmickBlock : public KdGameObject, public IGrabbable
{
public:

	GimmickBlock() {}
	~GimmickBlock() override {}

	void Init(const Math::Vector3& pos);
	void Update()		override;
	void PostUpdate()	override;
	void DrawLit()		override;

	void SetCarried(bool isCarried) override;
	bool IsCarried() const override { return m_isCarried; }
	BlockGridManager::BlockKind GetBlockKind() const override
	{
		return BlockGridManager::BlockKind::GimmickKey;
	}

	// 掴める対象として狙われる間、緑に点滅させる
	void SetGrabHighlight(bool enable) override { m_isGrabTargeted = enable; }

	void StartFall(const Math::Vector3& landingPos) override;

private:

	std::shared_ptr<KdModelWork> m_spModel = nullptr;

	bool m_isCarried = false;
	bool m_isGrabTargeted = false;
	int  m_blinkTimer = 0;

	bool m_isFalling = false;
	Math::Vector3 m_fallTarget = Math::Vector3::Zero;
	float m_fallSpeed = 0.0f;
};