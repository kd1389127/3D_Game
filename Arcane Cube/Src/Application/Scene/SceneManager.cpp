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
	// シーンが変わる時は、前のシーンのエフェクトを全部止める
	KdEffekseerManager::GetInstance().StopAllEffect();

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

void SceneManager::RequestChangeScene(SceneType _nextScene, int _nextStage, bool _whiteFade)
{
	// すでにフェード中なら何もしない(二重呼び出し防止)
	if (m_fadeState != FadeState::None) return;

	m_pendingScene = _nextScene;
	m_pendingStage = _nextStage;
	m_whiteFade = _whiteFade;
	m_fadeState = FadeState::Out;
}

void SceneManager::RequestFadeOnly()
{
	// すでにフェード中なら何もしない(落下中は毎フレーム呼ばれるので、これで1回だけになる)
	if (m_fadeState != FadeState::None) return;

	m_fadeOnly = true;
	m_fadeState = FadeState::Out;
}

bool SceneManager::ConsumeBlackoutEvent()
{
	const bool result = m_blackoutEvent;
	m_blackoutEvent = false;				// 読んだら下ろす(同じ暗転で2回リスポーンしないため)
	return result;
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

			if (m_fadeOnly)
			{
				// シーンはそのまま。「今、真っ暗」という事実だけ知らせる
				m_fadeOnly = false;
				m_blackoutEvent = true;
			}
			else
			{
				// 真っ暗になった瞬間にシーンを切り替える(切り替わりが見えない)
				if (m_pendingStage >= 0)
				{
					SetCurrentStage(m_pendingStage);
				}
				m_nextSceneType = m_pendingScene;
				ChangeScene(m_pendingScene);
			}

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
			m_whiteFade = false;	// 終わったら、次回のために黒に戻す
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

	// 画面全体を覆う白いフェードの時は白、普段は黒
	const float c = m_whiteFade ? 1.0f : 0.0f;
	Math::Color color = { c, c, c, m_fadeAlpha };
	sp.DrawBox(0, 0, 800, 450, &color, true);

	sp.End();
}