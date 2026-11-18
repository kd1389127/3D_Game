#pragma once
#include "BlockGridManager.h"

// BlockGrabberで持ち運べるブロックに共通のインターフェース
class IGrabbable
{
public:
	virtual ~IGrabbable() = default;

	virtual void SetCarried(bool isCarried) = 0;
	virtual bool IsCarried() const = 0;
	virtual void SetGrabHighlight(bool){}

	// 空中で離された時に、着地位置まで落とす(対応するブロックだけがoverrideする。何もしないのが標準)
	virtual void StartFall(const Math::Vector3&){}

	// BlockGridManagerに登録する際の種別
	virtual BlockGridManager::BlockKind GetBlockKind() const = 0;
};