#pragma once
#include"Option.h"
#include"UIManager.h"
#include"UI.h"
#include "Sound.h"
enum OptionTab {
	CommonTab,
	KeyboardTab,
	PadTab
};

class OptionMenuUI {
	Config config;
	Option option;
	UIManager uiManager;

	SoundManager* soundManager = nullptr;

	bool backRequested = false;
	bool rebuildUI = false;
	int time = 0;

	OptionTab currentTab = CommonTab;

	shared_ptr<Slider> master;
	shared_ptr<Slider> bgm;
	shared_ptr<Slider> se;
	shared_ptr<Slider> mouseSensitivity;
	shared_ptr<Slider> padSensitivity;
	shared_ptr<Toggle> invertY;
	shared_ptr<Toggle> invertX;

	void BuildUI();
	void SaveCurrentConfig();

	void Play(const string& name);
public:
	void Init();
	void Update();
	void Draw();

	bool IsBackRequested() const;
	void ResetBackRequest();

	void SetSoundManager(SoundManager* sound);
};