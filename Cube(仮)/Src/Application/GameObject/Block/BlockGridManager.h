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
		GimmickKey	// スイッチ用のギミック専用ブロック
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
	Math::Vector3 SnapToGrid(const Math::Vector3& pos)
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
	// baseCellを起点に、stepDir方向へrequestCount段分の設置予定座標を計算する
	// ★変更点：groundObjを受け取り、ブロック同士の重なりだけでなく
	//   壁・天井などのCOLとの衝突もチェックするようにした
	// ===================================================
	std::vector<Math::Vector3> TryStack(const Math::Vector3& baseCell, const Math::Vector3& stepDir,
		int requestCount, KdGameObject& groundObj) const
	{
		std::vector<Math::Vector3> result;
		result.reserve(requestCount);

		Math::Vector3 pos = baseCell;
		Math::Vector3 step = stepDir * GridSize;

		for (int i = 0; i < requestCount; ++i)
		{
			if (IsOccupied(pos)) break;
			if (!CanPlaceBlockAt(pos, groundObj)) break;

			result.push_back(pos);
			pos += step;
		}

		return result;
	}

	// そのマスに置かれているブロックがGimmickKeyかどうかチェック
	// (Switchが自分の真上のマスを毎フレーム調べるために使う)
	bool IsGimmickBlockAt(const Math::Vector3& snappedPos) const
	{
		auto it = m_occupied.find(ToKey(snappedPos));
		return it != m_occupied.end() && it->second == BlockKind::GimmickKey;
	}

	// そのマスを「使用中」として記録する(ブロックを置いた時に呼ぶ)
	// kindを省略した場合は従来通りNormalとして扱われる
	void Register(const Math::Vector3& snappedPos, BlockKind kind = BlockKind::Normal)
	{
		m_occupied[ToKey(snappedPos)] = kind;
	}

	// 指定したグリッドセル(スナップ済み座標)に、壁COLと重ならずにブロックを置けるかチェック
	// ※groundObjのIntersects()を使うことで、Groundの実際のワールド行列(スケール込み)が自動的に反映される
	bool CanPlaceBlockAt(const Math::Vector3& snappedPos, KdGameObject& groundObj) const
	{
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
	// baseCell自身がダメなら、法線方向前後にずらした候補も試す
	Math::Vector3 ResolvePlaceableCell(const Math::Vector3& hitPos, const Math::Vector3& axisNormal, KdGameObject& groundObj)
	{
		Math::Vector3 baseCell = SnapToGrid(hitPos + axisNormal * (GridSize * 0.5f + 0.01f));

		bool baseOccupied = IsOccupied(baseCell);
		bool baseCanPlace = CanPlaceBlockAt(baseCell, groundObj);
		KdDebugGUI::Instance().AddLog("[Grid] baseCell(%.1f,%.1f,%.1f) occupied=%d canPlace=%d\n",
			baseCell.x, baseCell.y, baseCell.z, baseOccupied, baseCanPlace);

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
	
	// ステージを読み込む際に前回置いたblockを無くす
	void Clear()
	{
		m_occupied.clear();
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

	// デバッグ用：現在位置周辺のグリッド線を描画するイメージ
	void DrawDebugGrid(KdDebugWireFrame& wire, const Math::Vector3& center, float range) const
	{
		constexpr Math::Color gridColor = { 0.2f, 0.8f, 1.0f, 1.0f }; // 見やすい水色

		// centerに一番近いグリッド線の開始位置を求める(常に8.0刻みの線を引くため)
		float startX = std::floor((center.x - range) / GridSize) * GridSize;
		float startZ = std::floor((center.z - range) / GridSize) * GridSize;

		// X方向に伸びる線(Z軸を固定して並べる)
		for (float z = startZ; z <= center.z + range; z += GridSize)
		{
			Math::Vector3 lineStart = { center.x - range, m_groundHeight, z };
			Math::Vector3 lineEnd = { center.x + range, m_groundHeight, z };
			wire.AddDebugLine(lineStart, lineEnd, gridColor);
		}

		// Z方向に伸びる線(X軸を固定して並べる)
		for (float x = startX; x <= center.x + range; x += GridSize)
		{
			Math::Vector3 lineStart = { x, m_groundHeight, center.z - range };
			Math::Vector3 lineEnd = { x, m_groundHeight, center.z + range };
			wire.AddDebugLine(lineStart, lineEnd, gridColor);
		}
	}

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
	float m_groundHeight = 0.0f;
};