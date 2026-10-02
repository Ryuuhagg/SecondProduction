//Input.h
#pragma once
#include"Option.h"

enum class Action {
	Confirm,
	Cancel,
	Jump,
	Dash,
	Pause,
	Up,
	Down,
	Left,
	Right,
	MoveUp,
	MoveDown,
	MoveLeft,
	MoveRight
};

class Input {
	static Config config;

	static char now[256];
	static char prev[256];

	static int padNow;
	static int padPrev;

	static float prevLX ;
	static float prevLY ;

	static float nowLX;
	static float nowLY;

	static int mouseNow;
	static int mousePrev;

	static int mouseX, mouseY;
	static int prevMouseX, prevMouseY;

	static int mouseDeltaX;
	static int mouseDeltaY;

public:
	static void Update();
	//押してる間発動
	static bool IsKeyPressed(int key) { return now[key]; }
	//押した瞬間だけ発動
	static bool IsKeyTrigger(int key) { return now[key] && !prev[key]; }
	//押してる間発動
	static bool IsPadPressed(int button){ return padNow & button; }
	//押した瞬間だけ発動
	static bool IsPadTrigger(int button){ return (padNow & button) && !(padPrev & button); }
	//アクションをPADとキーボードで共通させる
	static bool IsActionTrigger(Action action);
	static bool IsActionPressed(Action action);

	static bool IsValidBindKey(int key);
	// 押している間
	static bool IsMousePressed(int button) { return mouseNow & button; }
	// 押した瞬間
	static bool IsMouseTrigger(int button) { return (mouseNow & button) && !(mousePrev & button); }

	static bool IsMouseMoved() {
		return mouseX != prevMouseX || mouseY != prevMouseY;
	}

	static int GetMouseX() { return mouseX; }
	static int GetMouseY() { return mouseY; }

	static int GetMouseDeltaX() { return mouseDeltaX; }
	static int GetMouseDeltaY() { return mouseDeltaY; }

	static void CenterMouse();
	static void UpdateMouseDeltaFromCenter();

	static float GetMouseSensitivity() {
		return config.mouseSensitivity;
	}

	static float GetPadSensitivity() {
		return config.padSensitivity;
	}

	//カメラ反転フラグ
	static bool IsCamelaUpDownFlip() { return config.CamelaUpDownFlip; }
	static bool IsCamelaRightLeftFlip() { return config.CamelaRightLeftFlip; }

	//左スティック
	static float GetPadLX();
	static float GetPadLY();
	//右スティック
	static float GetPadRX();
	static float GetPadRY();
	//左スティックの動き
	static float GetAxisLX();
	static float GetAxisLY();
	//右スティックの動き
	static float GetAxisRX();
	static float GetAxisRY();
	//左スティック動かしたとき
	static bool IsStickTriggerUp();
	static bool IsStickTriggerDown();
	static bool IsStickTriggerLeft();
	static bool IsStickTriggerRight();

	static float ApplyDeadZone(float v);

	static void SetConfig(const Config& cfg);

	static int GetAnyKeyTrigger();
	static int GetAnyPadTrigger();
};