#include "GrabBeam.h"
#include "../../../Block/BlockGridManager.h"

namespace
{
	constexpr const char* kTexturePath = "Asset/Textures/Effect/PlasmaBeam.png";	// 置いた場所に合わせる

	constexpr int   kPointCount		= 9;		// 稲妻の点の数(両端を含む)
	constexpr int   kRefreshFrames	= 4;		// 何フレームごとに形を作り直すか(小さいほどバチバチ速い)
	constexpr float kAmplitudeRate	= 0.02f;	// ずれの最大量(稲妻の長さに対する割合)
	constexpr float kFadeSpeed		= 0.15f;	// 出る・消えるの速さ(1フレームに動く量)
	constexpr float kMinLength		= 0.5f;		// これより短いときは描かない
	constexpr float kStartInset     = 0.0f;		// 石の中心ではなく、石の表面あたりから出す距離
	constexpr float kEndInset		= BlockGridManager::GridSize * 0.5f;	// ブロックの中心ではなく、面の手前で止める
	constexpr float kMinVisibleRate = 0.001f;
	// 太さは「カメラからの距離 × この値」で決める(画面上でだいたい同じ太さに見える)
	constexpr float kOuterWidthPerDist = 0.03f;		// 外側の光のにじみ
	constexpr float kCoreWidthPerDist  = 0.008f;	// 中心の芯
	constexpr float kMinWidth          = 0.03f;		// 近くで細くなりすぎないための下限
	constexpr float kMaxWidth		   = 0.8f;		// 遠くで太くなりすぎないための上限

	

	// 1より大きい値で、眩しい光にする
	const Math::Color kOuterColor(0.3f, 1.0f, 0.4f, 0.6f);
	const Math::Color kCoreColor(1.0f, 1.5f, 1.0f, 1.0f);

	// -1〜1のランダムな値
	float RandomSigned()
	{
		return (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
	}
}

void GrabBeam::Init()
{
	// 幅は描くときに行列で決めるので、元の大きさは1×1
	m_spPolygon = std::make_shared<KdSquarePolygon>(kTexturePath);
	m_spPolygon->SetScale(Math::Vector2(1.0f, 1.0f));
}

void GrabBeam::Update(bool isActive, const Math::Vector3& startPos, const Math::Vector3& endPos, const Math::Vector3& eyePos)
{
	float targetRate = 0.0f;
	if (isActive)
	{
		targetRate = 1.0f;
	}
	m_rate += std::clamp(targetRate - m_rate, -kFadeSpeed, kFadeSpeed);

	m_startPos = startPos;
	m_eyePos = eyePos;
	if (isActive)
	{
		m_endPos = endPos;	// 離した後は更新せず、最後の位置へ向かって消える
	}

	if (m_rate <= 0.0f) { return; }

	// 稲妻の形を、kRefreshFramesごとに作り直す(これがバチバチ動く元)
	const bool needRefresh = m_offsets.empty() || (m_frame % kRefreshFrames == 0);
	if (needRefresh)
	{
		RefreshOffsets();
	}
	++m_frame;
}

// 各点の横ずれを、ランダムに決め直す
void GrabBeam::RefreshOffsets()
{
	m_offsets.resize(kPointCount);

	for (Math::Vector2& offset : m_offsets)
	{
		offset = Math::Vector2(RandomSigned(), RandomSigned());
	}
}

// 始点から終点まで、横にずらした点を並べる。短すぎる時は空のまま返す
void GrabBeam::BuildBoltPoints(std::vector<Math::Vector3>& outPoints) const
{
	outPoints.clear();

	const Math::Vector3 toEnd = m_endPos - m_startPos;
	const float fullLength = toEnd.Length();

	const float beamLength = fullLength - kStartInset - kEndInset;
	if (beamLength < kMinLength) { return; }

	const Math::Vector3 beamDir = toEnd / fullLength;
	const Math::Vector3 beamStart = m_startPos + beamDir * kStartInset;

	// 稲妻に直角な2方向(横ずれを付ける向き)
	Math::Vector3 refUp = Math::Vector3::Up;
	if (fabsf(beamDir.Dot(refUp)) > 0.95f)
	{
		refUp = Math::Vector3::Right;	// 真上・真下を向いている時は、別の向きで代用する
	}
	Math::Vector3 sideA = beamDir.Cross(refUp);
	sideA.Normalize();
	const Math::Vector3 sideB = beamDir.Cross(sideA);

	// 両端は0、真ん中で最大(sin)なので、端が杖とブロックにきれいにつく
	const float amplitude = beamLength * kAmplitudeRate;
	const int lastIndex = kPointCount - 1;

	for (int i = 0; i < kPointCount; ++i)
	{
		const float t = static_cast<float>(i) / lastIndex;
		const float envelope = sinf(t * DirectX::XM_PI);

		const Math::Vector2& offset = m_offsets[i];
		Math::Vector3 point = beamStart + beamDir * (beamLength * t);
		point += (sideA * offset.x + sideB * offset.y) * (amplitude * envelope);

		outPoints.push_back(point);
	}
}

void GrabBeam::Draw()
{
	if (!m_spPolygon || m_offsets.empty()) { return; }
	if (m_rate <= kMinVisibleRate) { return; }

	std::vector<Math::Vector3> points;
	points.reserve(kPointCount);
	BuildBoltPoints(points);
	if (points.empty()) { return; }

	// 出具合に合わせて、全体を薄くする
	Math::Color outerColor = kOuterColor;
	outerColor.w *= m_rate;

	Math::Color coreColor = kCoreColor;
	coreColor.w *= m_rate;

	// 太い光(にじみ)と細い光(芯)を重ねる
	DrawLayer(points, kOuterWidthPerDist, outerColor);
	DrawLayer(points, kCoreWidthPerDist, coreColor);
}

// 点と点の間を、カメラの方を向いた細長い板でつなぐ
void GrabBeam::DrawLayer(const std::vector<Math::Vector3>& points, float widthPerDist, const Math::Color& color) 
{
	auto& shader = KdShaderManager::Instance().m_StandardShader;
	shader.SetOnlyEmissive(false);	// 魔法石の描画で付くフラグが残っていても、板が黒くならないように
	shader.SetFogEnable(false);		// 霧で暗くならないようにする

	for (size_t i = 0; i + 1 < points.size(); ++i)
	{
		const Math::Vector3 segment = points[i + 1] - points[i];
		const float segmentLength = segment.Length();
		if (segmentLength < 0.0001f) { continue; }

		const Math::Vector3 segmentDir = segment / segmentLength;
		const Math::Vector3 center = (points[i] + points[i + 1]) * 0.5f;

		// カメラから遠い板ほど太くする(近い=杖の根元は細く、遠い=ブロック側は太く)
		const float distToEye = (m_eyePos - center).Length();
		const float width = std::clamp(widthPerDist * distToEye, kMinWidth, kMaxWidth);

		// 板の幅の向き = 線にも、カメラへの向きにも直角
		Math::Vector3 widthDir = segmentDir.Cross(m_eyePos - center);
		if (widthDir.LengthSquared() < 0.0001f) { continue; }	// 線がカメラの真正面の時は描かない
		widthDir.Normalize();

		const Math::Vector3 normalDir = segmentDir.Cross(widthDir);

		// つなぎ目が隙間なく重なるよう、長さを少し伸ばす。ただし両端(石・ブロック側)は伸ばさない
		const bool isFirst = (i == 0);
		const bool isLast = (i + 2 == points.size());

		float startExtend = width * 0.5f;
		if (isFirst) { startExtend = 0.0f; }

		float endExtend = width * 0.5f;
		if (isLast) { endExtend = 0.0f; }

		const float quadLength = segmentLength + startExtend + endExtend;
		const Math::Vector3 quadCenter = center + segmentDir * ((endExtend - startExtend) * 0.5f);

		// 板の行列:X = 線の向き(長さ)、Y = 幅の向き(太さ)、Z = 板の向き
		Math::Matrix mat = Math::Matrix::Identity;
		mat.Right(segmentDir * quadLength);
		mat.Up(widthDir * width);
		mat.Backward(normalDir);
		mat.Translation(quadCenter);

		shader.DrawPolygon(*m_spPolygon, mat, color);
	}
}