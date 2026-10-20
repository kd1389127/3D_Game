#pragma once

class BaseScene
{
public :

	BaseScene()			 { Init(); }
	virtual ~BaseScene() {}

	void PreUpdate();
	void Update();
	void PostUpdate();

	void PreDraw();
	void Draw();
	void DrawSprite();
	void DrawDebug();

	// オブジェクトリストを取得
	const std::list<std::shared_ptr<KdGameObject>>& GetObjList()
	{
		return m_objList;
	}
	
	// オブジェクトリストに追加
	void AddObject(const std::shared_ptr<KdGameObject>& _obj)
	{
		m_objList.push_back(_obj);
	}

protected :

	// 継承先シーンで必要ならオーバーライドする
	virtual void Event();
	virtual void Init();

	// ポーズ中かどうか。ポーズを使うシーンだけオーバーライドする
	virtual bool IsPaused() const { return false; }
	// ポーズ中かどうか。1フレームの最初(PreUpdate)で1回だけ決めて、
	// Update・PostUpdateでも同じ値を使う(途中で変わると、カメラだけ1フレーム遅れてずれるため)
	bool m_paused = false;

	// ポーズ中でも更新を止めないオブジェクト(ポーズメニュー本体)
	std::shared_ptr<KdGameObject> m_pauseObj;

	// 全オブジェクトのアドレスをリストで管理
	std::list<std::shared_ptr<KdGameObject>> m_objList;
};
