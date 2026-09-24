#include "Reticle.h"
#include "../../Character/Player/Player.h"

void Reticle::Init()
{
	m_texNormal.Load("Asset/Textures/Reticle/Reticle1.png");
	m_texDelete.Load("Asset/Textures/Reticle/Reticle_Red.png");
}

void Reticle::DrawSprite()
{
	bool isDeleteMode = false;
	if (auto player = m_wpPlayer.lock())
	{
		isDeleteMode = player->IsBlockDeleteMode();
	}

	KdTexture& tex = isDeleteMode ? m_texDelete : m_texNormal;
	KdShaderManager::Instance().m_spriteShader.DrawTex(&tex, 0, 0, 64, 64);
}
