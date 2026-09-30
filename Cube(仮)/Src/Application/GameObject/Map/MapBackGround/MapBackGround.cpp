#include "MapBackGround.h"

void MapBackGround::Init()
{
	if (!m_spModel)
	{
		m_spModel = std::make_shared<KdModelData>();
		m_spModel->Load("Asset/Models/Map/MapBackGround/MapBackGround.gltf");
	}
	
	m_mWorld = Math::Matrix::CreateScale(m_scale) 
		* Math::Matrix::CreateRotationY(DirectX::XMConvertToRadians(m_yawDeg));
}

void MapBackGround::Update()
{
	auto target = m_wpTarget.lock();
	if (!target) { return; }

	// XZだけ追従し、Yは固定(地平線と地面の高さを合わせるため)
	Math::Vector3 pos = target->GetPos();
	m_mWorld.Translation(Math::Vector3(pos.x, m_baseY, pos.z));
}

void MapBackGround::DrawUnLit()
{
	if (m_spModel)
	{
		KdShaderManager::Instance().m_StandardShader.DrawModel(*m_spModel, m_mWorld);
	}
}
