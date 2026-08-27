#pragma once

#include "../Grabbable.h"

class NormalBlock : public KdGameObject, public IGrabbable
{
public:

	NormalBlock() {}
	~NormalBlock()		override {}

	void Init(const Math::Vector3& pos);
	void PostUpdate() override;
	void DrawLit()	override;

	void SetCarried(bool isCarried);
	bool IsCarried() const { return m_isCarried; }
	BlockGridManager::BlockKind GetBlockKind() const override
	{
		return BlockGridManager::BlockKind::Normal;
	}

private:

	std::shared_ptr<KdModelWork> m_spModel = nullptr;

	bool m_isCarried = false;
};