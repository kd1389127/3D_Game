#pragma once

class TitleTexture : public KdGameObject
{
public:
	TitleTexture(){}
	~TitleTexture()		override{}

	void Init()			override;
	void Update()		override;
	void DrawSprite()	override;

private:

	KdTexture m_bgTex;		// 背景
	KdTexture m_logoTex;	// タイトル
	KdTexture m_startTex;	// Click To Start

	int m_frame = 0;
};