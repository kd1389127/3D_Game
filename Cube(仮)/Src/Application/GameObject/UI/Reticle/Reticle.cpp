#include "Reticle.h"

void Reticle::Init()
{
	m_tex.Load("Asset/Textures/Reticle/Reticle1.png");
}

void Reticle::DrawSprite()
{
	KdShaderManager::Instance().m_spriteShader.DrawTex(&m_tex, 0, 0, 64, 64);
}
