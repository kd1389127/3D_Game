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
	void PostUpdate() override;
	void DrawLit() override;

	void SetCarried(bool isCarried) override;
	bool IsCarried() const override { return m_isCarried; }
	BlockGridManager::BlockKind GetBlockKind() const override
	{
		return BlockGridManager::BlockKind::GimmickKey;
	}

private:

	std::shared_ptr<KdModelWork> m_spModel = nullptr;

	bool m_isCarried = false;
};