#include "CameraBase.h"
#include "../Map/MapBlock/MapBlock.h"

void CameraBase::Init()
{
	if (!m_spCamera)
	{
		m_spCamera = std::make_shared<KdCamera>();
	}
	//// ↓画面中央座標
	//m_FixMousePos.x = 640;
	//m_FixMousePos.y = 360;
}

void CameraBase::PreDraw()
{
	if (!m_spCamera) { return; }

	m_spCamera->SetCameraMatrix(m_mWorld);
	MapBlock::UpdateFrustum(m_spCamera->GetCameraViewMatrix(), m_spCamera->GetProjMatrix());
	m_spCamera->SetToShader();
}

void CameraBase::SetTarget(const std::shared_ptr<KdGameObject>& target)
{
	if (!target) { return; }

	m_wpTarget = target;
}