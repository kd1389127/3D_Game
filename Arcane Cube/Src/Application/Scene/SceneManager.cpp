#include "SceneManager.h"

#include "BaseScene/BaseScene.h"
#include "TitleScene/TitleScene.h"
#include "GameScene/GameScene.h"
#include "ResultScene/ResultScene.h"

void SceneManager::PreUpdate()
{
	// フェードの進行(暗転が終わったらここでシーンが切り替わる)
	UpdateFade();	

	if (m_currentSceneType != m_nextSceneType || m_forceReload)
	{
		ChangeScene(m_nextSceneType);
		m_forceReload = false;
	}

	m_currentScene->PreUpdate();
}

void SceneManager::Update()
{
	m_currentScene->Update();
}

void SceneManager::PostUpdate()
{
	m_currentScene->PostUpdate();
}

void SceneManager::PreDraw()
{
	m_currentScene->PreDraw();
}

void SceneManager::Draw()
{
	m_currentScene->Draw();
}

void SceneManager::DrawSprite()
{
	m_currentScene->DrawSprite();

	// フェードは一番手前に描く
	DrawFade();
}

void SceneManager::DrawDebug()
{
	m_currentScene->DrawDebug();
}

const std::list<std::shared_ptr<KdGameObject>>& SceneManager::GetObjList()
{
	return m_currentScene->GetObjList();
}

void SceneManager::AddObject(const std::shared_ptr<KdGameObject>& _obj)
{
	m_currentScene->AddObject(_obj);
}

void SceneManager::ChangeScene(SceneType _sceneType)
{
	// 次のシーンを作成し、現在のシーンにする
	switch (_sceneType)
	{
	case SceneType::Title:
		m_currentScene = std::make_shared<TitleScene>();
		break;
	case SceneType::Game:
		m_currentScene = std::make_shared<GameScene>();
		break;
	case SceneType::Result:
		m_currentScene = std::make_shared<ResultScene>();
		break;

	}

	// 現在のシーン情報を更新
	m_currentSceneType = _sceneType;
}

void SceneManager::RequestChangeScene(SceneType _nextScene, int _nextStage)
{
	// すでにフェード中なら何もしない(二重呼び出し防止)
	if (m_fadeState != FadeState::None) return;

	m_pendingScene = _nextScene;
	m_pendingStage = _nextStage;
	m_fadeState = FadeState::Out;
}

void SceneManager::UpdateFade()
{
	switch (m_fadeState)
	{
	case FadeState::None:
		break;

	case FadeState::Out:
		m_fadeAlpha += kFadeSpeed;
		if (m_fadeAlpha >= 1.0f)
		{
			m_fadeAlpha = 1.0f;

			// 真っ暗になった瞬間にシーンを切り替える(切り替わりが見えない)
			if (m_pendingStage >= 0)
			{
				m_currentStage = m_pendingStage;
			}
			m_nextSceneType = m_pendingScene;
			ChangeScene(m_pendingScene);	// 同じ種類でも必ず作り直す

			m_holdTimer = kHoldFrames;
			m_fadeState = FadeState::Hold;
		}
		break;

	case FadeState::Hold:
		if (--m_holdTimer <= 0)
		{
			m_fadeState = FadeState::In;
		}
		break;

	case FadeState::In:
		m_fadeAlpha -= kFadeSpeed;
		if (m_fadeAlpha <= 0.0f)
		{
			m_fadeAlpha = 0.0f;
			m_fadeState = FadeState::None;
		}
		break;
	}
}

void SceneManager::DrawFade()
{
	if (m_fadeAlpha <= 0.0f) return;

	auto& sp = KdShaderManager::Instance().m_spriteShader;
	sp.Begin();
	sp.SetMatrix(Math::Matrix::Identity);

	// 画面全体を覆う黒い四角(画面より十分大きくしておく)
	Math::Color color = { 0.0f, 0.0f, 0.0f, m_fadeAlpha };
	sp.DrawBox(0, 0, 800, 450, &color, true);

	sp.End();
}