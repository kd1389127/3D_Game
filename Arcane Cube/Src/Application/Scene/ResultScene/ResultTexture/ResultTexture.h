#pragma once

class ResultTexture : public KdGameObject
{
public:
	ResultTexture() {}
	~ResultTexture()	override {}

	void Init()			override;
	void Update()		override;
	void DrawSprite()	override;

private:

	KdTexture m_bgTex;		// 背景
	KdTexture m_logoTex;	// GAME CLEAR
	KdTexture m_keyTex;		// Press Any Key

	int m_frame = 0;
};