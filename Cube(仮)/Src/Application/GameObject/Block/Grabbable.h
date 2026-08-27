#pragma once
#include "BlockGridManager.h"

// BlockGrabberで持ち運べるブロックに共通のインターフェース
class IGrabbable
{
public:
	virtual ~IGrabbable() = default;

	virtual void SetCarried(bool isCarried) = 0;
	virtual bool IsCarried() const = 0;

	// BlockGridManagerに登録する際の種別
	virtual BlockGridManager::BlockKind GetBlockKind() const = 0;
};