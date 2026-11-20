#pragma once

#include"../BaseScene/BaseScene.h"

class Player;
class Guide;
class PauseMenu;

class GameScene : public BaseScene
{
public :

	GameScene()  { Init(); }
	~GameScene() {}

private:

	void Event() override;
	void Init()  override;
	bool IsPaused() const override;

	std::shared_ptr<PauseMenu> m_pauseMenu;
	std::shared_ptr<Guide>	   m_guide;

	// 撮影モード(デバッグ用)：UIと杖を消して、プレイヤーを自由に飛ばす
	void EnterPhotoMode();
	bool m_isPhotoMode = false;
	std::weak_ptr<Player> m_wpPlayer;
	std::vector<std::shared_ptr<KdGameObject>> m_photoHideObjs;	// 撮影中に隠すもの
};
