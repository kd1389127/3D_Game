#pragma once

class Ground : public KdGameObject
{
public:

	Ground()				{}
	~Ground()		override{}

	void Init(const std::string& modelPath, float scale);
	void DrawLit()	override;

private:

	std::shared_ptr<KdModelWork> m_spModel = nullptr;

};