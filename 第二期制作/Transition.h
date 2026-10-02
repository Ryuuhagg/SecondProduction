//Transition.h
#pragma once
#include"Constant.h"
#include"Image.h"
#include"Timer.h"
enum class TransitionState {
	None,
	Enter,      // 演出開始
	Switching,  // 切り替えタイミング
	Stay,		// シーン移動以外の用途の切り替えタイミング
	Exit,       // 演出終了中
	Finished,	// ここでSceneの初期化処理を起動
	End,		// 初期化をしたくないときはこっち
};

class Transition {
protected:
	TransitionState state = TransitionState::Enter;
public:
	virtual void Update() = 0;
	virtual void Draw() = 0;
	virtual bool IsFinished() { return state == TransitionState::Finished; }
	virtual bool IsEnd() { return state == TransitionState::End; }
	virtual TransitionState GetState() { return state; };

	void StartExit() {
		state = TransitionState::Exit;
	}
};

class Fade : public Transition{
private:
	int alpha = 0;
public:
	void Update() override;
	void Draw() override;
	bool IsMidPoint() { return alpha >= 255; }
};

class Slide : public Transition {
private:
	int current = -WIDTH;
public:
	void Update() override;
	void Draw() override;
};

class Goal : public Transition {
	int img;
	Stopwatch wait;
public:
	Goal();
	void Update() override;
	void Draw() override;
};

class DamageFade : public Transition {
	int alpha = 0;
	int life;
	int life_img;
	Stopwatch wait;
public:
	DamageFade(int life);
	void Update() override;
	void Draw() override;
};

class MoveFade : public Transition {
private:
	int alpha = 0;
public:
	void Update() override;
	void Draw() override;
	bool IsMidPoint() { return alpha >= 255; }
};