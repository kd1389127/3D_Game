#pragma once
#include <unordered_map>
#include <cmath>
#include <vector>

// マップ上を「マス目(グリッド)」で管理するためのクラス
// ブロックがどのマスに置かれているかを記録し、重複配置を防ぐ役割
class BlockGridManager
{
public:
	// 置かれているブロックの種類
	enum class BlockKind
	{
		Normal,		// 弾で生成する通常ブロック
		GimmickKey,	// スイッチ用のギミック専用ブロック
		Map,		// CSVでのMapブロック用
	};

	// 1マスのサイズ(ブロック1個分の大きさに合わせてある)
	static constexpr float GridSize = 8.0f;

	// シングルトン(ゲーム中どこからでも同じ1つのインスタンスにアクセスできる)
	static BlockGridManager& Instance()
	{
		static BlockGridManager inst;
		return inst;
	}

	// ステージ切り替え時にGameScene::Initから呼ぶ：そのステージの地面の高さをグリッドの基準にする
	void SetGroundHeight(float groundHeight)
	{
		m_groundHeight = groundHeight;
	}
	
	// 自由な座標(小数)を、一番近いマスの中心座標に丸める(スナップ)
	// 例：(9.8, 4.3, -3.1) → 一番近いマスの中心座標に変換
	Math::Vector3 SnapToGrid(const Math::Vector3& pos) const
	{
		constexpr float halfGrid = GridSize * 0.5f;

		return Math::Vector3
		(
			std::round(pos.x / GridSize) * GridSize,
			// Yだけ「マスの中心が地面から半マス浮いた位置」になるようオフセットしてから丸めている
			std::round((pos.y - m_groundHeight - halfGrid) / GridSize) * GridSize + halfGrid + m_groundHeight,
			std::round(pos.z / GridSize) * GridSize
		);

	}

	// 指定したマス(スナップ済み座標)に、すでに何か置かれているかチェック
	bool IsOccupied(const Math::Vector3& snappedPos) const
	{
		return m_occupied.count(ToKey(snappedPos)) > 0;
	}

	// ===================================================
	// baseCellを起点に、stepDir方向へrequestCount段分の「置ける座標」を返す
	// 重複・壁や天井のCOL・(checkPlayer=trueなら)プレイヤーの体のどれかに
	// 引っかかった段で打ち切る
	// ===================================================
	std::vector<Math::Vector3> TryStack(const Math::Vector3& baseCell, const Math::Vector3& stepDir,
		int requestCount, KdGameObject& groundObj, bool checkPlayer = false) const
	{
		std::vector<Math::Vector3> result;
		result.reserve(requestCount);

		Math::Vector3 pos = baseCell;
		Math::Vector3 step = stepDir * GridSize;

		for (int i = 0; i < requestCount; ++i)
		{
			if (IsOccupied(pos)) break;
			if (!CanPlaceBlockAt(pos, groundObj)) break;
			if (checkPlayer && IsBlockedByPlayer(pos)) break;		// 体に重なる段で打ち切る

			result.push_back(pos);
			pos += step;
		}

		return result;
	}

	// そのマスを「使用中」として記録する(ブロックを置いた時に呼ぶ)
	// kindを省略した場合は従来通りNormalとして扱われる
	void Register(const Math::Vector3& snappedPos, BlockKind kind = BlockKind::Normal)
	{
		m_occupied[ToKey(snappedPos)] = kind;
	}

	// 指定したマス(スナップ済み座標)に、ブロックを置けるかチェックする
	// 軽い判定から順に行い、最後に壁・天井などのCOLとの重なりを調べる
	// ※groundObjのIntersects()は、Groundの実際のワールド行列(スケール込み)を自動で反映してくれる
	bool CanPlaceBlockAt(const Math::Vector3& snappedPos, KdGameObject& groundObj) const
	{
		// 一番軽い判定を先に。ここで落とせば、下の当たり判定まで進まなくて済む
		if (!IsInsideBuildBounds(snappedPos)) return false;	// Map外には置けない
		if (IsInForbiddenZone(snappedPos)) return false;	// 禁止エリアには置けない

		if (IsOverlappingFreeBlock(snappedPos)) return false;	// ギミックブロックと重なる所には置けない

		// 高く積めるエリアの中なら、そのエリア用の層数を使う
		int limitLayers = m_buildLimitLayers;
		if (m_tallLimitLayers > 0 &&
			snappedPos.x >= m_tallZoneMin.x && snappedPos.x <= m_tallZoneMax.x &&
			snappedPos.z >= m_tallZoneMin.y && snappedPos.z <= m_tallZoneMax.y)
		{
			limitLayers = m_tallLimitLayers;
		}

		if (limitLayers > 0)
		{
			const float cellTop = snappedPos.y + GridSize * 0.5f;
			if (cellTop > m_groundHeight + GridSize * limitLayers + 0.01f) return false;
		}

		constexpr float checkMargin = 0.4f;
		Math::Vector3 halfExtents(GridSize * 0.5f * checkMargin, GridSize * 0.5f * checkMargin, GridSize * 0.5f * checkMargin);

		KdCollider::BoxInfo checkBox(
			KdCollider::TypeGround | KdCollider::TypeBump,
			Math::Matrix::CreateTranslation(snappedPos),
			Math::Vector3::Zero,
			halfExtents,
			false
		);

		// KdGameObject::Intersectsは内部でm_mWorld(Groundの実際のスケール・位置)を使ってくれる
		return !groundObj.Intersects(checkBox, nullptr);
	}

	// 弾の着弾情報から、実際に置けるセルを候補の中から探す
	// 候補の順番：着弾点の手前(baseCell) → 法線の外側 → 法線の内側
	// どれもダメならbaseCellを返す
	Math::Vector3 ResolvePlaceableCell(const Math::Vector3& hitPos, const Math::Vector3& axisNormal, KdGameObject& groundObj)
	{
		Math::Vector3 baseCell = SnapToGrid(hitPos + axisNormal * (GridSize * 0.5f + 0.01f));
	
		if (!IsOccupied(baseCell) && CanPlaceBlockAt(baseCell, groundObj))
		{
			return baseCell;
		}

		Math::Vector3 outerCell = baseCell + axisNormal * GridSize;
		if (!IsOccupied(outerCell) && CanPlaceBlockAt(outerCell, groundObj))
		{
			return outerCell;
		}

		Math::Vector3 innerCell = baseCell - axisNormal * GridSize;
		if (!IsOccupied(innerCell) && CanPlaceBlockAt(innerCell, groundObj))
		{
			return innerCell;
		}

		return baseCell;
	}

	// そのマスの「使用中」記録を消す(ブロックを持ち上げた時に呼ぶ)
	void Unregister(const Math::Vector3& snappedPos)
	{
		m_occupied.erase(ToKey(snappedPos));
	}
	
	// グリッドに登録しない「自由に動くブロック」(ギミックブロック)を覚えておく
	void RegisterFreeBlock(const std::shared_ptr<KdGameObject>& block)
	{
		m_freeBlocks.push_back(block);
	}

	// 指定した座標に、自由に動くブロック(ギミックブロック)が重なっているか
	// ignoreを渡すと、そのブロック自身は数えない
	// (魔法ブロックをギミックブロックの上に生成させないために使う)
	bool IsOverlappingFreeBlock(const Math::Vector3& pos,const KdGameObject* ignore = nullptr) const
	{
		constexpr float limit = GridSize - 0.5f;		// 1マス分より少し小さい値(触れているだけは重なりにしない)
		for (auto& wp : m_freeBlocks)
		{
			auto sp = wp.lock();
			if (!sp || sp.get() == ignore) continue;

			Math::Vector3 d = sp->GetPos() - pos;
			if (fabsf(d.x) < limit && fabsf(d.y) < limit && fabsf(d.z) < limit) return true;
		}
		return false;
	}

	// ステージを読み込む際に、前のステージの情報(ブロック・体・禁止エリア・置ける範囲)を全部消す
	void Clear()
	{
		m_occupied.clear();
		m_freeBlocks.clear();
		m_hasPlayerBody = false;	// ステージ切替で前のプレイヤー位置を引きずらない
		m_forbiddenZones.clear();	// ステージが変わったら、前のステージの禁止エリアを持ち越さない
		m_hasBounds = false;
	}

	// 法線ベクトル(斜めを含む)を、一番近いXYZ軸方向(±1方向のどれか)に丸める
	// 例：弾が斜めの角に当たっても、「一番近い面」の向きとして扱いたい時に使う
	static Math::Vector3 SnapNormalToAxis(const Math::Vector3& normal)
	{
		float ax = fabsf(normal.x), ay = fabsf(normal.y), az = fabsf(normal.z);

		// X成分が一番大きいならX軸方向、Y成分が一番大きいならY軸方向…という判定
		if (ax >= ay && ax >= az) return Math::Vector3(normal.x > 0 ? 1.f : -1.f, 0, 0);
		if (ay >= ax && ay >= az) return Math::Vector3(0, normal.y > 0 ? 1.f : -1.f, 0);
		return Math::Vector3(0, 0, normal.z > 0 ? 1.f : -1.f);
	}

	// デバッグ用：グリッドの各マスを「ブロック形状の枠線」で描画する(高さ方向も含む)
	void DrawDebugGrid(KdDebugWireFrame& wire, const Math::Vector3& center, float range) const
	{
		if (!m_showDebugGrid) { return; }

		constexpr float half = GridSize * 0.5f;
		constexpr Math::Color gridColor = { 0.2f, 0.8f, 1.0f, 1.0f };
		constexpr Math::Color axisXColor = { 1.0f, 0.2f, 0.2f, 1.0f };
		constexpr Math::Color axisZColor = { 0.2f, 0.4f, 1.0f, 1.0f };
		constexpr Math::Color normalColor = { 1.0f, 1.0f, 0.2f, 1.0f };
		constexpr Math::Color gimmickColor = { 1.0f, 0.3f, 1.0f, 1.0f };
		constexpr Math::Color mapColor = { 0.45f, 0.45f, 0.45f, 1.0f };

		// マスの「境界線」の高さ(=ブロックの底面の高さ)
		const float y = m_groundHeight + m_debugLayer * GridSize;

		// 線を引くマスの範囲(マス中心の座標で持つ)
		// Mapの範囲が分かっていればそれに合わせ、無ければcenterの周りだけ描く
		float minCellX = 0.0f;
		float maxCellX = 0.0f;
		float minCellZ = 0.0f;
		float maxCellZ = 0.0f;

		if (m_hasBounds)
		{
			minCellX = m_boundsMin.x;
			maxCellX = m_boundsMax.x;
			minCellZ = m_boundsMin.y;	// Vector2のyにはZが入っている
			maxCellZ = m_boundsMax.y;
		}
		else
		{
			const Math::Vector3 centerCell = SnapToGrid(center);
			const float rangeLength = static_cast<int>(range / GridSize) * GridSize;
			minCellX = centerCell.x - rangeLength;
			maxCellX = centerCell.x + rangeLength;
			minCellZ = centerCell.z - rangeLength;
			maxCellZ = centerCell.z + rangeLength;
		}

		// 線の描画範囲(マス中心から半マス外側が境界)
		const float minX = minCellX - half;
		const float maxX = maxCellX + half;
		const float minZ = minCellZ - half;
		const float maxZ = maxCellZ + half;

		// 境界線の本数 = マスの数 + 1
		const int cellCountX = static_cast<int>(std::round((maxX - minX) / GridSize));
		const int cellCountZ = static_cast<int>(std::round((maxZ - minZ) / GridSize));

		// Z方向へ伸びる線(Xの位置ごとに1本)
		for (int i = 0; i <= cellCountX; ++i)
		{
			const float x = minX + i * GridSize;
			wire.AddDebugLine({ x, y, minZ }, { x, y, maxZ }, gridColor);
		}

		// X方向へ伸びる線(Zの位置ごとに1本)
		for (int i = 0; i <= cellCountZ; ++i)
		{
			const float z = minZ + i * GridSize;
			wire.AddDebugLine({ minX, y, z }, { maxX, y, z }, gridColor);
		}

		// ワールド原点の軸(マスの中心を通る線)
		wire.AddDebugLine({ 0, y, minZ }, { 0, y, maxZ }, axisZColor);
		wire.AddDebugLine({ minX, y, 8}, { maxX, y, 8 }, axisXColor);

		// 置き済みブロックのマス
		for (const auto& [key, kind] : m_occupied)
		{
			auto [kx, ky, kz] = key;
			Math::Vector3 cellPos(kx * GridSize, ky * GridSize + half + m_groundHeight, kz * GridSize);

			Math::Color c = normalColor;
			if (kind == BlockKind::GimmickKey)  c = gimmickColor;
			else if (kind == BlockKind::Map)	c = mapColor;

			Math::Matrix mat = Math::Matrix::CreateTranslation(cellPos);
			wire.AddDebugBox(mat, Math::Vector3(half, half, half), Math::Vector3::Zero, false, c);
		}
		/*if (m_hasPlayerBody)
		{
			wire.AddDebugBox(Math::Matrix::CreateTranslation(m_playerBodyCenter),
				m_playerBodyHalf, Math::Vector3::Zero, false, playerColor);
		}*/
	}

	// 表示する高さの段を切り替える(0=地面の高さ)
	void ChangeDebugLayer(int delta)
	{
		m_debugLayer += delta;
		if (m_debugLayer < 0) { m_debugLayer = 0; }
	}

	void ToggleDebugGrid()
	{
		m_showDebugGrid = !m_showDebugGrid;
	}

	bool IsDebugGridVisible() const
	{
		return m_showDebugGrid;
	}

	// プレイヤーの体(箱)を毎フレーム教えてもらう。Magicwand::Updateから呼ぶ
	// center=箱の中心、halfExtents=箱の半分のサイズ(幅・高さ・奥行き)
	void SetPlayerBody(const Math::Vector3& center, const Math::Vector3& halfExtents)
	{
		m_playerBodyCenter = center;
		m_playerBodyHalf = halfExtents;
		m_hasPlayerBody = true;
	}

	// そのマスが、プレイヤーの体と重なっているか(箱と箱の重なり判定)
	bool OverlapsPlayerBody(const Math::Vector3& cellPos) const
	{
		if (!m_hasPlayerBody) return false;		// 体の情報が無いうちは「重なっていない」扱い

		constexpr float half = GridSize * 0.5f; // マスの半分のサイズ
		constexpr float eps = 0.5f;				// 触れているだけは「重なり」にしない

		// マスの中心とプレイヤーの中心のずれ
		const Math::Vector3 d = cellPos - m_playerBodyCenter;

		// 3軸すべてで「ずれ < マスの半分 + 体の半分 - eps」なら重なっている
		// (1軸でも離れていれば重ならない)
		return fabsf(d.x) < half + m_playerBodyHalf.x - eps &&
			fabsf(d.y) < half + m_playerBodyHalf.y - eps &&
			fabsf(d.z) < half + m_playerBodyHalf.z - eps;
	}

	// プレイヤーの足元が「そのマスの上面」以上にいるか(=ブロックを足の下に置ける状態か)
	bool IsPlayerAboveCell(const Math::Vector3& cellPos) const
	{
		constexpr float tolerance = 1.0f;	// 上面より少し下でも許す(跳ぶ途中でも置ける余裕)
		const float cellTop = cellPos.y + GridSize * 0.5f;
		const float feetY = m_playerBodyCenter.y - m_playerBodyHalf.y;
		return feetY >= cellTop - tolerance;
	}

	// 体と重なっていて、かつ足の下に置ける状態でもない = 置けない
	bool IsBlockedByPlayer(const Math::Vector3& cellPos) const
	{
		return OverlapsPlayerBody(cellPos) && !IsPlayerAboveCell(cellPos);
	}
	
	// 魔法ブロックを積める最大の層数(0=上限なし)。StageDataのbuildLimitLayersを渡す
	void SetBuildLimitLayers(int layers) { m_buildLimitLayers = layers; }

	// このXZの範囲(minXZ〜maxXZ、yにはZが入る)の中だけ、積める層数をlayersにする
	void SetTallBuildZone(int layers, const Math::Vector2& minXZ, const Math::Vector2& maxXZ)
	{
		m_tallLimitLayers = layers;
		m_tallZoneMin = minXZ;
		m_tallZoneMax = maxXZ;
	}

	// 魔法ブロックを置ける範囲(XZ)を決める。MapLoaderがCSVの広さから自動で呼ぶ
	void SetBuildBounds(float minX, float maxX, float minZ, float maxZ)
	{
		m_boundsMin = Math::Vector2(minX, minZ);
		m_boundsMax = Math::Vector2(maxX, maxZ);
		m_hasBounds = true;
	}

	// 位置が、置ける範囲の中か。margin分だけ外側まで許す(プレイヤーのMap外判定にも使う)
	bool IsInsideBuildBounds(const Math::Vector3& pos, float margin = 0.0f) const
	{
		if (!m_hasBounds) return true;	// 範囲が未設定なら、何も制限しない
		constexpr float eps = 0.01f;	// 小数の誤差で、端のマスが外れないように
		return pos.x >= m_boundsMin.x - margin - eps && pos.x <= m_boundsMax.x + margin + eps &&
			pos.z >= m_boundsMin.y - margin - eps && pos.z <= m_boundsMax.y + margin + eps;
	}

	// 「ここには置けない」箱を足す。StageDataの禁止エリア(GameScene)と、階段のマス(MapLoader)から登録する
	void AddForbiddenZone(const Math::Vector3& minPos, const Math::Vector3& maxPos)
	{
		m_forbiddenZones.push_back({ minPos, maxPos });
	}

	// 位置が、登録した禁止エリアのどれかの中か
	bool IsInForbiddenZone(const Math::Vector3& pos) const
	{
		for (const auto& zone : m_forbiddenZones)
		{
			if (pos.x >= zone.min.x && pos.x <= zone.max.x &&
				pos.y >= zone.min.y && pos.y <= zone.max.y &&
				pos.z >= zone.min.z && pos.z <= zone.max.z) return true;
		}
		return false;
	}

	// 自由に動くブロック(ギミックブロック)の一覧。スイッチが「誰が乗っているか」を調べるために使う
	const std::vector<std::weak_ptr<KdGameObject>>& GetFreeBlocks() const { return m_freeBlocks; }

private:
	// 座標(小数)を、マス目上の「整数の住所」に変換する(重複チェック用のキー)
	// 同じマスなら必ず同じ(x,y,z)の組になるようにしている
	std::tuple<int, int, int> ToKey(const Math::Vector3& pos) const
	{
		constexpr float halfGrid = GridSize * 0.5f;

		return
		{
			(int)std::round(pos.x / GridSize),
			(int)std::round((pos.y - m_groundHeight - halfGrid) / GridSize),
			(int)std::round(pos.z / GridSize)
		};
	}

	// unordered_setで tuple<int,int,int> を使うためのハッシュ計算関数
	// (どのマスが埋まっているかを高速に検索できるようにするための下準備)
	struct KeyHash
	{
		size_t operator()(const std::tuple<int, int, int>& k) const
		{
			auto [x, y, z] = k;
			return std::hash<int>()(x) ^ (std::hash<int>()(y) << 1) ^ (std::hash<int>()(z) << 2);
		}
	};

	// 「使用中のマス」を全部記録しておくコンテナ
	std::unordered_map<std::tuple<int, int, int>, BlockKind, KeyHash> m_occupied;
	std::vector<std::weak_ptr<KdGameObject>> m_freeBlocks;
	float m_groundHeight = 0.0f;
	bool  m_showDebugGrid = false;
	int   m_debugLayer = 0;
	int	  m_buildLimitLayers = 0;

	Math::Vector3 m_playerBodyCenter = Math::Vector3::Zero;
	Math::Vector3 m_playerBodyHalf = Math::Vector3::Zero;
	bool          m_hasPlayerBody = false;

	int           m_tallLimitLayers = 0;
	Math::Vector2 m_tallZoneMin = Math::Vector2::Zero;
	Math::Vector2 m_tallZoneMax = Math::Vector2::Zero;

	// 置ける範囲(XZ)。yにはZが入る(m_tallZoneMinと同じ使い方)
	Math::Vector2 m_boundsMin = Math::Vector2::Zero;
	Math::Vector2 m_boundsMax = Math::Vector2::Zero;
	bool          m_hasBounds = false;

	// 置けない箱
	struct ForbiddenZoneBox { Math::Vector3 min; Math::Vector3 max; };
	std::vector<ForbiddenZoneBox> m_forbiddenZones;
};