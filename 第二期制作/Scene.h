//Scene.h
#pragma once
#include"UIManager.h"
#include"Option.h"
#include"Input.h"
#include"OptionMenuUI.h"
#include"Timer.h"
class SceneManager;

class Scene {
protected:
	UIManager uiManager;
	bool first;
public:
	virtual void Init() = 0;
	virtual void Update(SceneManager& manager) = 0;
	virtual void Draw() = 0;
	//2026-07-10: UIをInitにそのまま書くと見ずらくなるのでここに書く
	virtual void UISet() = 0;
};
// タイトルシーン
class Title :public Scene {
	Config config;
	Option option;

	int changeFlg;
	int img;
public:
	Title();
	void UISet() override;
	void Init() override;
	void Update(SceneManager& manager) override;
	void Draw() override;

};
// マップ選択シーン
class Select :public Scene {
	//サムネハンドル
	int thumbnail_img = -1;
	int changeFlg;
	// 2026-05-18: タイトル画面で遊ぶCSVマップを選べるようにするため追加。
	int selectedMapIndex;
	// 2026-06-15: タイトルから遊ぶ時、START/GOALが床の上にないマップを止める警告用。
	bool titleMapFloorWarning = false;
	// 2026-07-09: マップのスタートとゴールがつながっていなかったら警告を出す。
	bool showStartGoalWarning = false;
	//bool confirmSaveOnTitle;
	void ApplySelectedMap();
	//サムネをロードする関数
	void LoadSelectedThumbnail();
public:
	Select();
	~Select();
	void UISet() override;
	void Init() override;
	void Update(SceneManager& manager) override;
	void Draw() override;

};

enum GamePauseMode {
	PauseMenu,
	OptionInGame
};

enum class GameLaunchMode {
	NormalPlay,
	EditorTestPlay
};

enum class GameResult {
	Win,
	Lose
};
// 迷路をプレイするシーン
class Game :public Scene {
	GameLaunchMode launchMode;
	bool isPause;
	bool goTitle;
	bool confirmSaveOnTitle;
	bool restart;
	bool goOption;
	//別クラスで設定のやつ作成
	OptionMenuUI optionUI;
	bool showingOption = false;
	//ポーズ画面とオプション切替用
	GamePauseMode pauseMode = PauseMenu;
	UIManager option;

	Stopwatch timer;
	int elapsed;
public:
	Game(GameLaunchMode launchMode = GameLaunchMode::NormalPlay);
	void UISet() override;
	void Init() override;
	void Update(SceneManager& manager) override;
	void Draw() override;
	void DrawOption();
	void BuildPauseUI();

	bool IsGoal();
};
// リザルトシーン
class Result :public Scene {
	GameLaunchMode launchMode;
	GameResult result;
	int clearTime = 0;
	int goalImg = 0;
	int totalTime = 0;

	int elapsed = 0;
	int minutes = 0;

	int changeflg = 0;

	float goalScale = 0.0f;
	bool goalScaleBack = false;

	int countTimer = 0;
	const int countDuration = 60;

	void ScaleUpAnimation();
	void CountUp();
public:
	Result(int time, GameLaunchMode launchMode = GameLaunchMode::NormalPlay, GameResult result = GameResult::Win);
	void UISet() override;
	void Init() override;
	void Update(SceneManager& manager) override;
	void Draw() override;

	bool ScaleUpEnd() const { return goalScaleBack && goalScale == 1.0f; }
};
// マップ制作シーン
class Create : public Scene {
	// 2026-05-18: エディター画面からタイトルへ戻るボタンの押下状態を持つため追加。
	bool goTitle = false;
	// 2026-
	// : エディターで作った現在のマップを、そのままゲームシーンで遊ぶため追加。
	bool playCurrentMap = false;
	// 2026-06-15: START/GOALが床の上にない時、このマップで遊べないことを警告するため追加。
	bool playMapFloorWarning = false;
	// 2026-05-27: エディター変更後にタイトルへ戻る前、保存確認を出すため追加。
	bool confirmSaveOnTitle = false;
public:
	Create();
	void UISet() override;
	void Init()override;
	void Update(SceneManager& manager) override;
	void Draw() override;
};
// 設定シーン
class OptionMenu : public Scene {
	//別クラスで設定のやつ作成
	OptionMenuUI optionUI;
public:
	OptionMenu();
	void UISet() override;
	void Init()override;
	void Update(SceneManager& maneger)override;
	void Draw()override;
};