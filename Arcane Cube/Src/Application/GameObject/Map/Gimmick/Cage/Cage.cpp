#include "Cage.h"
#include "../Switch/Switch.h"
#include "../../../Block/BlockGridManager.h"

// 全部のスイッチがオンの間だけ開く(スイッチ1個なら { sw } と渡す)
void Cage::Init(const std::vector<std::shared_ptr<Switch>>& switches, 
	const Math::Vector3& basePos, const Math::Vector3& scale, 
	float maxHeight, float speed, float yawDeg)
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

		// 幅と厚みは見た目に合わせ、高さだけ極端に長くして、上から入れないようにする
		// (モデルの高さ1が57倍されるので、Extents.yの9だと足元から約1,000の高さになる)
		DirectX::BoundingBox wallBox;
		wallBox.Center = DirectX::XMFLOAT3(0.0f, 9.0f, 0.0f);
		wallBox.Extents = DirectX::XMFLOAT3(0.5f, 9.0f, 0.1f);
		m_pCollider->RegisterCollisionShape("CageWall", wallBox, KdCollider::TypeBump);
	}


	// --- ここからCage::Initの最後に追加 ---
	// 檻の中のマスにも、魔法ブロックを置かせない
	// 当たり判定の半分の大きさ(ローカル)：幅0.5×scale.x、厚み0.1×scale.z
	const float yawRad = DirectX::XMConvertToRadians(yawDeg);
	const float lx = 0.5f * scale.x;
	const float lz = 0.1f * scale.z;

	// 回転後の「ワールドで見た半分の大きさ」(Y回転だけなので、cosとsinの絶対値で足せばよい)
	const float hx = fabsf(cosf(yawRad)) * lx + fabsf(sinf(yawRad)) * lz;
	const float hz = fabsf(sinf(yawRad)) * lx + fabsf(cosf(yawRad)) * lz;

	// マスの「中心」で判定するので、マスの半分(4)だけ広げておく
	constexpr float cellHalf = BlockGridManager::GridSize * 0.5f;
	BlockGridManager::Instance().AddForbiddenZone(
		Math::Vector3(basePos.x - hx - cellHalf, basePos.y - cellHalf, basePos.z - hz - cellHalf),
		Math::Vector3(basePos.x + hx + cellHalf, basePos.y + scale.y + cellHalf, basePos.z + hz + cellHalf));

	Math::Matrix scaleMat = Math::Matrix::CreateScale(scale);
	Math::Matrix rotMat = Math::Matrix::CreateRotationY(DirectX::XMConvertToRadians(yawDeg));
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
		// プレイヤーが下をくぐれる高さ(身長10＋余裕2)。全開まで待たずに判定を消す
		static constexpr float kPassHeight = 12.0f;

		// maxHeightがそれより低いMapでも消えるように、小さい方を使う
		const float passHeight = std::min(kPassHeight, m_maxHeight);

		const bool canPass = (m_currentHeight >= passHeight);
		m_pCollider->SetEnableAll(!canPass);
	}
}

void Cage::DrawLit()
{
	if (!m_spModel) return;

	KdShaderManager::Instance().m_StandardShader.DrawModel(*m_spModel, m_mWorld);
}