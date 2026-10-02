//Input.cpp
#include"Input.h"
#include"DxLib.h"
#include"Constant.h"
#include <cmath>

char Input::now[256];
char Input::prev[256];

int Input::padNow = 0;
int Input::padPrev = 0;

float Input::prevLX = 0.0f;
float Input::prevLY = 0.0f;

float Input::nowLX = 0.0f;
float Input::nowLY = 0.0f;

int Input::mouseNow = 0;
int Input::mousePrev = 0;

int Input::mouseDeltaX = 0;
int Input::mouseDeltaY = 0;

int Input::mouseX = 0;
int Input::mouseY = 0;

int Input::prevMouseX = 0;
int Input::prevMouseY = 0;

Config Input::config;

void Input::Update() {
	memcpy(prev, now, 256);
    GetHitKeyStateAll(now);

	padPrev = padNow;
	padNow = GetJoypadInputState(DX_INPUT_PAD1);

    prevLX = nowLX;
    prevLY = nowLY;

    nowLX = GetPadLX();
    nowLY = GetPadLY();

    mousePrev = mouseNow;
    mouseNow = GetMouseInput();

    prevMouseX = mouseX;
    prevMouseY = mouseY;
    GetMousePoint(&mouseX, &mouseY);
}
//アクションごとにキーを決定
bool Input::IsActionTrigger(Action action) {
    switch (action) {
    case Action::Confirm:
        return IsKeyTrigger(config.KeyConfirm) || IsPadTrigger(config.PadConfirm);

    case Action::Cancel:
        return IsKeyTrigger(config.KeyCancel) || IsPadTrigger(config.PadCancel);

    case Action::Jump:
        return IsKeyTrigger(config.KeyJump) || IsPadTrigger(config.PadJump);

    case Action::Pause:
        return IsKeyTrigger(config.KeyPause)||IsPadTrigger(config.PadPause);

    case Action::Up:
        return IsKeyTrigger(KEY_INPUT_UP)
            || IsPadTrigger(PAD_INPUT_UP)
            || IsStickTriggerUp();

    case Action::Down:
        return IsKeyTrigger(KEY_INPUT_DOWN)
            || IsPadTrigger(PAD_INPUT_DOWN)
            || IsStickTriggerDown();


    case Action::Right:
        return IsKeyTrigger(KEY_INPUT_RIGHT)
            || IsPadTrigger(PAD_INPUT_RIGHT)
            || IsStickTriggerRight();

    case Action::Left:
        return IsKeyTrigger(KEY_INPUT_LEFT)
            || IsPadTrigger(PAD_INPUT_LEFT)
            || IsStickTriggerLeft();
    }

    return false;
}
bool Input::IsActionPressed(Action action) {
    switch (action) {
    case Action::Dash:
        return IsKeyPressed(config.KeyDash) || IsPadPressed(config.PadDash);

    case Action::Up:
        return IsKeyPressed(KEY_INPUT_UP)
            || IsPadPressed(PAD_INPUT_UP)
            || nowLY <= -0.5f;

    case Action::Down:
        return IsKeyPressed(KEY_INPUT_DOWN)
            || IsPadPressed(PAD_INPUT_DOWN)
            || nowLY >= 0.5f;

    case Action::Right:
        return IsKeyPressed(KEY_INPUT_RIGHT)
            || IsPadPressed(PAD_INPUT_RIGHT)
            || nowLX >= 0.5f;

    case Action::Left:
        return IsKeyPressed(KEY_INPUT_LEFT)
            || IsPadPressed(PAD_INPUT_LEFT)
            || nowLX <= -0.5f;

    case Action::MoveUp:
        return IsKeyPressed(config.KeyFront);

    case Action::MoveDown:
        return IsKeyPressed(config.KeyBack);

    case Action::MoveRight:
        return IsKeyPressed(config.KeyRight);

    case Action::MoveLeft:
        return IsKeyPressed(config.KeyLeft);
    }

    return false;
}
//許可されてるキー
bool Input::IsValidBindKey(int key) {
    return KeyToStringSafe(key) != "UNKNOWN";
}

float Input::GetPadLX() {
    int x, y;
    GetJoypadAnalogInput(&x, &y, DX_INPUT_PAD1);
    return x / 1000.0f;
}

float Input::GetPadLY() {
    int x, y;
    GetJoypadAnalogInput(&x, &y, DX_INPUT_PAD1);
    return y / 1000.0f;
}

float Input::GetPadRX() {
    int x, y;
    GetJoypadAnalogInputRight(&x, &y, DX_INPUT_PAD1);
    return x / 1000.0f;
}

float Input::GetPadRY() {
    int x, y;
    GetJoypadAnalogInputRight(&x, &y, DX_INPUT_PAD1);
    return y / 1000.0f;
}

float Input::GetAxisLX() {
    float x = 0;

    // キーボード
    if (IsActionPressed(Action::MoveRight)) x += 1.0f;
    if (IsActionPressed(Action::MoveLeft)) x -= 1.0f;

    // パッド
    float lx = GetPadLX();

    lx = ApplyDeadZone(lx);

    // 強い方を使う
    if (fabs(lx) > fabs(x)) x = lx;

    return x;
}

float Input::GetAxisLY() {
    float y = 0;

    // キーボード
    if (IsActionPressed(Action::MoveUp)) y += 1.0f;
    if (IsActionPressed(Action::MoveDown)) y -= 1.0f;

    // パッド
    float ly = GetPadLY();

    // 上がマイナスなので反転
    ly = -ly;

    ly = ApplyDeadZone(ly);

    if (fabs(ly) > fabs(y)) y = ly;

    return y;
}

float Input::GetAxisRX() {
    float x = 0;

    // キーボード
    if (IsKeyPressed(KEY_INPUT_RIGHT)) x -= 1.0f;
    if (IsKeyPressed(KEY_INPUT_LEFT)) x += 1.0f;

    // パッド
    float rx = GetPadRX();

    rx = -rx;

    rx = ApplyDeadZone(rx);
    // 強い方を使う
    if (fabs(rx) > fabs(x)) x = rx;

    return x;
}

float Input::GetAxisRY() {
    float y = 0;

    // キーボード
    if (IsKeyPressed(KEY_INPUT_UP)) y += 1.0f;
    if (IsKeyPressed(KEY_INPUT_DOWN)) y -= 1.0f;

    // パッド
    float ry = GetPadRY();

    // 上がマイナスなので反転
    ry = -ry;

    ry = ApplyDeadZone(ry);

    if (fabs(ry) > fabs(y)) y = ry;

    return y;
}

bool Input::IsStickTriggerUp() {
    const float trigger = 0.6f;
    const float release = 0.3f;

    return prevLY > -release && nowLY <= -trigger;
}

bool Input::IsStickTriggerDown() {
    const float trigger = 0.6f;
    const float release = 0.3f;

    return prevLY < release && nowLY >= trigger;
}

bool Input::IsStickTriggerLeft() {
    const float trigger = 0.6f;
    const float release = 0.3f;

    return prevLX > -release && nowLX <= -trigger;
}

bool Input::IsStickTriggerRight() {
    const float trigger = 0.6f;
    const float release = 0.3f;

    return prevLX < release && nowLX >= trigger;
}

float Input::ApplyDeadZone(float v) {
    float dead = 0.2f;
    return (fabs(v) < dead) ? 0.0f : v;
}

void Input::SetConfig(const Config& cfg) {
    config = cfg;
}

int Input::GetAnyKeyTrigger() {
    for (int i = 0; i < 256; i++) {
        if (now[i] && !prev[i]) {
            return i;
        }
    }
    return -1;
}

int Input::GetAnyPadTrigger() {
    int pad = GetJoypadInputState(DX_INPUT_PAD1);

    int diff = pad & (~padPrev);

    if (diff) return diff;

    return -1;
}

void Input::CenterMouse()
{
    SetMousePoint(WIDTH / 2, HEIGHT / 2);
}

void Input::UpdateMouseDeltaFromCenter()
{
    int mx, my;
    GetMousePoint(&mx, &my);

    mouseDeltaX = mx - WIDTH / 2;
    mouseDeltaY = my - HEIGHT / 2;

    SetMousePoint(WIDTH / 2, HEIGHT / 2);
}