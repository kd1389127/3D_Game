#pragma once

class Reticle : public KdGameObject
{
public:

	Reticle() {}
	~Reticle()			override {}

	void Init()			override;
	void DrawSprite()	override;

private:

	KdTexture m_tex;
};
