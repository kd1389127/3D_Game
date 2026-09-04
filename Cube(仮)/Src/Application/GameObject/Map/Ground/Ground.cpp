#include "Ground.h"

void Ground::Init(const std::string& modelPath, float scale)
{
	if (!m_spModel)
	{
		m_spModel = std::make_shared<KdModelWork>();
		m_spModel->SetModelData(modelPath);

		m_pCollider = std::make_unique<KdCollider>();
		m_pCollider->RegisterCollisionShape("Ground", m_spModel, KdCollider::TypeGround | KdCollider::TypeBump);
	}

	Math::Matrix sclaeMat = Math::Matrix::CreateScale(scale);
	m_mWorld = sclaeMat;

	// デバック
	//{
	//	std::string dumpName = "collision_dump_" + modelPath.substr(modelPath.find_last_of("/\\") + 1) + ".txt";
	//	std::ofstream ofs(dumpName);

	//	auto spData = m_spModel->GetData();
	//	auto& originalNodes = spData->GetOriginalNodes();

	//	std::set<float> xs;

	//	for (int nodeIdx : spData->GetCollisionMeshNodeIndices())
	//	{
	//		const auto& node = originalNodes[nodeIdx];
	//		if (!node.m_spMesh) continue;

	//		// ノードのローカルワールド行列 × Groundのスケール = 本当のゲーム内ワールド行列
	//		Math::Matrix worldMat = node.m_worldTransform * m_mWorld;

	//		for (auto& pos : node.m_spMesh->GetVertexPositions())
	//		{
	//			Math::Vector3 worldPos = Math::Vector3::Transform(pos, worldMat);

	//			float rounded = std::round(worldPos.x * 100.0f) / 100.0f;
	//			xs.insert(rounded);

	//			float xmod8 = std::fmod(std::fmod(worldPos.x, 8.0f) + 8.0f, 8.0f);

	//			ofs << "node=" << node.m_name
	//				<< " X=" << worldPos.x
	//				<< " Y=" << worldPos.y
	//				<< " Z=" << worldPos.z
	//				<< " (Xmod8=" << xmod8 << ")\n";
	//		}
	//	}

	//	ofs.close();

	//	KdDebugGUI::Instance().AddLog("[Dump] %s : unique X count=%zu -> %s\n",
	//		modelPath.c_str(), xs.size(), dumpName.c_str());
	//}

}

void Ground::DrawLit()
{
	if (!m_spModel) return;

	KdShaderManager::Instance().m_StandardShader.DrawModel(*m_spModel,m_mWorld);
}
