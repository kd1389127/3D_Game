#pragma once

class BaseScene;

class SceneManager
{
public :

	// シーン情報
	enum class SceneType
	{
		Title,
		Game,
		Result,
	};

	void PreUpdate();
	void Update();
	void PostUpdate();

	void PreDraw();
	void Draw();
	void DrawSprite();
	void DrawDebug();

	// 現在のシーンのオブジェクトリストを取得
	const std::list<std::shared_ptr<KdGameObject>>& GetObjList();

	// 現在のシーンにオブジェクトを追加
	void AddObject(const std::shared_ptr<KdGameObject>& _obj);

	int GetCurrentStage() const { return m_currentStage; }
	void SetCurrentStage(int stage) { m_currentStage = stage; }

	void ReloadScene() { m_forceReload = true; } // 同じ種類のシーンでも強制的に作り直す

	// フェード付きでシーンを切り替える(暗転してから切り替わり、明るくなる)
	// _nextStageを0以上にすると、切り替える瞬間にステージ番号も変更する
	// フェード中に呼ばれた要求は無視する(ゴールが毎フレーム呼んでも1回だけ処理される)
	void RequestChangeScene(SceneType _nextScene, int _nextStage = -1);

	bool IsFading() const { return m_fadeState != FadeState::None; }

private :

	// マネージャーの初期化
	// インスタンス生成(アプリ起動)時にコンストラクタで自動実行
	void Init()
	{
		// 開始シーンに切り替え
		ChangeScene(m_currentSceneType);
	}

	// シーン切り替え関数
	void ChangeScene(SceneType _sceneType);

	// 現在のシーンのインスタンスを保持しているポインタ
	std::shared_ptr<BaseScene> m_currentScene = nullptr;

	// 現在のシーンの種類を保持している変数
	SceneType m_currentSceneType = SceneType::Title;
	
	// 次のシーンの種類を保持している変数
	SceneType m_nextSceneType = m_currentSceneType;

	int m_currentStage = 0;

	bool m_forceReload = false;

	// ===== フェード =====
	enum class FadeState
	{
		None,	// フェードしていない
		Out,	// だんだん暗くなる
		Hold,	// 真っ暗のまま少し待つ(この間にシーンを切り替え済み)
		In,		// だんだん明るくなる
	};

	void UpdateFade();
	void DrawFade();

	// 起動直後はフェードインから始める
	FadeState m_fadeState = FadeState::In;
	float m_fadeAlpha = 1.0f;	// 0=透明 1=真っ暗

	int m_holdTimer = 0;

	// フェード完了時に切り替える予定のシーンとステージ
	SceneType m_pendingScene = SceneType::Title;
	int m_pendingStage = -1;

	static constexpr float kFadeSpeed = 1.0f / 30.0f;	// 約0.5秒(60FPS時)で暗転
	static constexpr int kHoldFrames = 15;				// 真っ暗で待つフレーム数

private:

	SceneManager() { Init(); }
	~SceneManager() {}

public:

	// シングルトンパターン
	// 常に存在する && 必ず1つしか存在しない(1つしか存在出来ない)
	// どこからでもアクセスが可能で便利だが
	// 何でもかんでもシングルトンという思考はNG
	static SceneManager& Instance()
	{
		static SceneManager instance;
		return instance;
	}
};
