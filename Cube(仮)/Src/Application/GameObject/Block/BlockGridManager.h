#pragma once
#include <unordered_set>
#include <cmath>

// マップ上を「マス目(グリッド)」で管理するためのクラス
// ブロックがどのマスに置かれているかを記録し、重複配置を防ぐ役割
class BlockGridManager
{
public:
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

	// そのマスを「使用中」として記録する(ブロックを置いた時に呼ぶ)
	void Register(const Math::Vector3& snappedPos)
	{
		m_occupied.insert(ToKey(snappedPos));
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
	std::unordered_set<std::tuple<int, int, int>, KeyHash> m_occupied;
	float m_groundHeight = 0.0f;
};