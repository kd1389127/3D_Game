#pragma once

// スイッチ起動用の専用ブロック
// 通常ブロック(NormalBlock)と同様にグリッドに設置・持ち運びされるが、
// BlockGridManager上では種別(GimmickKey)として区別して登録される
class GimmickBlock : public KdGameObject
{
public:

	GimmickBlock() {}
	~GimmickBlock() override {}

	void Init(const Math::Vector3& pos);
	void PostUpdate() override;
	void DrawLit() override;

	void SetCarried(bool isCarried);
	bool IsCarried() const { return m_isCarried; }

private:

	std::shared_ptr<KdModelWork> m_spModel = nullptr;

	bool m_isCarried = false;
};