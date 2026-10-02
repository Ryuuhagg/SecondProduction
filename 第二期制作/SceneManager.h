//SceneManager.h
#pragma once
#include"Scene.h"
#include"Transition.h"
#include"Option.h"
#include"Sound.h"
#include<memory>
using namespace std;
class SceneManager {
	unique_ptr<Scene> current;
	unique_ptr<Scene> next;
	unique_ptr<Transition> transition;

	SoundManager soundManager;
public:
	SoundManager& GetSoundManager();

	void Update();
	void Draw();
	void ChangeScene(unique_ptr<Scene> newScene,
					 unique_ptr<Transition> trans = nullptr);
	void Trans(unique_ptr<Transition> trans);
};