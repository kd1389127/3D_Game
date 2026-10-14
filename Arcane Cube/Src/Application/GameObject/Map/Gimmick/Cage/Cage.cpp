#include "Cage.h"
#include "../Switch/Switch.h"

// 全部のスイッチがオンの間だけ開く(スイッチ1個なら { sw } と渡す)
void Cage::Init(const std::vector<std::shared_ptr<Switch>>& switches, 
	const Math::Vector3& basePos, 
	const Math::Vector3& scale, 
	float maxHeight, 
	float speed)
{
	m_switches = switches;
	m_basePos = basePos;
	m_maxHeight = maxHeight;
	m_speed = speed;

	if (!m_spModel)
	{
		m_spModel = std::make_shared<KdModelWork>();
		m_spModel->SetModelData("Asset/Models/Map/Cage1/Cage1.gltf");
	}

	if (!m_pCollider)
	{
		m_pCollider = std::make_unique<KdCollider>();

		// 閉じている間はプレイヤーを通さない壁として機能させる
		DirectX::BoundingBox wallBox;
		wallBox.Center = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
		wallBox.Extents = DirectX::XMFLOAT3(2.0f, 3.0f, 0.1f);
		m_pCollider->RegisterCollisionShape("CageWall", wallBox, KdCollider::TypeBump);
	}

	Math::Matrix scaleMat = Math::Matrix::CreateScale(scale);
	Math::Matrix rotMat   = Math::Matrix::CreateRotationY(DirectX::XMConvertToRadians(90));
	Math::Matrix transMat = Math::Matrix::CreateTranslation(basePos);

	m_mWorld = scaleMat * rotMat * transMat;
}

void Cage::Update()
{
	// 全部のスイッチがオンのときだけ開く
	// 最初は「開く」と仮定して、1つでもオフのスイッチがあれば「閉じる」に変える
	bool active = !m_switches.empty();
	for (auto& sw : m_switches)
	{
		if (!sw || !sw->IsActive())
		{
			active = false;
			break;
		}
	}
	float target = active ? m_maxHeight : 0.0f;

	// 1フレームごとにm_speed分だけ近づける(DeltaTimeは使わない、他クラスと同じ固定フレーム前提の書き方)
	if (m_currentHeight < target)
	{
		m_currentHeight = std::min(m_currentHeight + m_speed, target);
	}
	else if (m_currentHeight > target)
	{
		m_currentHeight = std::max(m_currentHeight - m_speed, target);
	}

	Math::Vector3 pos = m_basePos;
	pos.y += m_currentHeight;
	SetPos(pos);

	// 完全に上がりきったら通行できるよう当たり判定を無効化、
	// 下がっている間(閉じている間)は有効化する
	if (m_pCollider)
	{
		bool fullyOpen = (m_currentHeight >= m_maxHeight - 0.001f);
		m_pCollider->SetEnableAll(!fullyOpen);
	}
}

void Cage::DrawLit()
{
	if (!m_spModel) return;

	KdShaderManager::Instance().m_StandardShader.DrawModel(*m_spModel, m_mWorld);
}