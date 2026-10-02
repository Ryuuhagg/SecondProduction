//UIManager.h
#pragma once
#include<vector>
#include<memory>
#include"UI.h"
#include"Sound.h"
using namespace std;
class UIManager {
	vector<shared_ptr<UI>> uiList;
	int scrollY = 0;
	int focusIndex = 0;
	bool enableScroll = true;

	SoundManager* soundManager = nullptr;

	void Play(const string& name);
public:
	void Add(const shared_ptr<UI>& ui);
	void Update();
	void Draw();
	void Clear();
	int GetScrollY() { return scrollY; }
	void SetScrollY(int y) {
		scrollY = y;

		if (scrollY < 0) scrollY = 0;
		if (scrollY > 500) scrollY = 500;
	}
	void SetScrollEnabled(bool flag) { enableScroll = flag; }

	void SetSoundManager(SoundManager* sound);
};