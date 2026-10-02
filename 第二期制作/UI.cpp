//UI.cpp
#include"UI.h"
#include"DxLib.h"
#include"Option.h"
#include"Input.h"
#include"FontManager.h"

bool UI::IsMouseOver(int x, int y, int w, int h, int offsetY)
{
	int mx = Input::GetMouseX();
	int my = Input::GetMouseY();

	int drawY = y - offsetY;

	return (mx >= x && mx < x + w &&
		my >= drawY && my <= drawY + h);
}
#pragma region Button
Button::Button(Pos pos, int sizeX, int sizeY): UI(pos,0,sizeX,sizeY),clicked(false){}

void Button::Update(int offsetY) {
	clicked = IsClicked(offsetY);

	if (clicked && onClick) { 
		onClick();
	}

	bool hover = IsMouseOver(pos.x, pos.y , sizeX, sizeY, offsetY);
	color = hover ? GetColor(200, 200, 255) : GetColor(150, 150, 255);
}

void Button::Draw(int offsetY) {
	int drawColor = isFocused
		? GetColor(255, 255, 0)  
		: color;

	DrawBox
		(pos.x
		, pos.y - offsetY
		, pos.x + sizeX
		, pos.y + sizeY - offsetY
		, drawColor, true
		);
}

bool Button::IsClicked(int offsetY) {
	return IsMouseOver(pos.x, pos.y, sizeX, sizeY, offsetY)
		&& Input::IsMouseTrigger(MOUSE_INPUT_LEFT);
}

bool Button::IsUsing() {
	return clicked;
}

void Button::SetOnClick(function<void()> func) {
	onClick = func;
}

void Button::OnClick() {
	if (onClick) onClick();
}
#pragma endregion
#pragma region Toggle
Toggle::Toggle(Pos pos, int sizeX, int sizeY) : UI(pos, 0, sizeX, sizeY), isOn(false),usingNow(false) {}

void Toggle::Update(int offsetY) {
	usingNow = false;
	bool hover = IsMouseOver(pos.x, pos.y , sizeX, sizeY ,offsetY);
	color = hover ? GetColor(200, 200, 255) : GetColor(150, 150, 255);
	if (IsClicked(offsetY)) {
		OnClick();
		usingNow = true;
	}
}

void Toggle::Draw(int offsetY) {
	color = isOn ? GetColor(100, 255, 100) : GetColor(255, 100, 100);

	int drawColor = isFocused
		? GetColor(255, 255, 0)
		: color;

	DrawBox(pos.x
		, pos.y - offsetY
		, pos.x + sizeX
		, pos.y + sizeY - offsetY
		, drawColor
		, true
	);

	const char* state = isOn ? "ON" : "OFF";
	DrawString(pos.x + 10, pos.y - offsetY + 5, state, GetColor(0, 0, 0));
}

void Toggle::SetOnClick(function<void()> func) {
	onClick = func;
}

void Toggle::OnClick() {
	if (onClick)onClick();
	isOn = !isOn;
}

bool Toggle::IsClicked(int offsetY) {
	return IsMouseOver(pos.x, pos.y, sizeX, sizeY, offsetY)
		&& Input::IsMouseTrigger(MOUSE_INPUT_LEFT);
}

bool Toggle::IsUsing() {
	return usingNow;
}
#pragma endregion
#pragma region Slider
Slider::Slider(Pos pos, int w, int h):UI(pos, GetColor(200,200,200), w, h), dragging(false), value(0.5f) {}

void Slider::Update(int offsetY) {
	int mx = Input::GetMouseX();
	int my = Input::GetMouseY();

	int drawY = pos.y - offsetY;
	int knobX = pos.x + (int)(value * sizeX);

	bool onKnob =
		(mx >= knobX - 8 && mx <= knobX + 8 &&
			my >= drawY - 8 && my <= drawY + sizeY + 8);

	bool onBar =
		(mx >= pos.x && mx <= pos.x + sizeX &&
			my >= drawY && my <= drawY + sizeY);

	if (Input::IsMouseTrigger(MOUSE_INPUT_LEFT) && (onKnob || onBar)) {
		dragging = true;
	}

	if (!Input::IsMousePressed(MOUSE_INPUT_LEFT)) {
		dragging = false;
	}

	if (isFocused) {
		if (Input::IsKeyTrigger(KEY_INPUT_RIGHT)) {
			value += 0.05f;
		}
		if (Input::IsKeyTrigger(KEY_INPUT_LEFT)) {
			value -= 0.05f;
		}

		if (value < 0) value = 0;
		if (value > 1) value = 1;
	}

	if (dragging) {
		value = (float)(mx - pos.x) / sizeX;
		if (value < 0.0f) value = 0.0f;
		if (value > 1.0f) value = 1.0f;
	}
}

void Slider::Draw(int offsetY) {
	// ノブ位置
	int knobX = pos.x + (int)(value * sizeX);

	int drawColor = isFocused
		? GetColor(255, 255, 0)
		: color;

	// バー
	DrawBox(pos.x
		, pos.y - offsetY
		,knobX
		, pos.y + sizeY - offsetY
		, drawColor
		, TRUE
	);

	DrawBox(knobX, pos.y - offsetY,
		pos.x + sizeX, pos.y + sizeY - offsetY,
		drawColor, TRUE);

	// ノブ
	DrawBox(knobX - 5, pos.y - 5 - offsetY,
		knobX + 5, pos.y + sizeY + 5 - offsetY,
		GetColor(255, 255, 255), TRUE);
}

bool Slider::IsUsing() {
	return dragging;
}
#pragma endregion
#pragma region VerticalSlider
VerticalSlider::VerticalSlider(Pos pos, int w, int h) :UI(pos, GetColor(200, 200, 200), w, h), dragging(false), value(0.5f) {}

void VerticalSlider::Update(int offsetY) {
	int mx = Input::GetMouseX();
	int my = Input::GetMouseY();

	int drawY = pos.y - offsetY;
	int knobY = drawY + (int)(value * sizeY);

	bool onKnob =
		(mx >= pos.x - 8 && mx <= pos.x + 8 &&
			my >= drawY - 8 && my <= drawY + sizeY + 8);

	bool onBar =
		(mx >= pos.x && mx <= pos.x + sizeX &&
			my >= drawY && my <= drawY + sizeY);

	if (Input::IsMouseTrigger(MOUSE_INPUT_LEFT) && (onKnob || onBar)) {
		dragging = true;
	}

	if (!Input::IsMousePressed(MOUSE_INPUT_LEFT)) {
		dragging = false;
	}

	if (isFocused) {
		if (Input::IsKeyTrigger(KEY_INPUT_RIGHT)) {
			value += 0.05f;
		}
		if (Input::IsKeyTrigger(KEY_INPUT_LEFT)) {
			value -= 0.05f;
		}

		if (value < 0) value = 0;
		if (value > 1) value = 1;
	}

	if (dragging) {
		value = (float)(my - pos.y) / sizeY;
		if (value < 0.0f) value = 0.0f;
		if (value > 1.0f) value = 1.0f;
	}
}

void VerticalSlider::Draw(int offsetY) {
	// ノブ位置
	int drawY = pos.y - offsetY;
	int knobY = drawY + (int)(value * sizeY);

	int barColor = GetColor(120, 120, 120);
	int knobColor = GetColor(255, 255, 255);

	DrawBox(
		pos.x,
		drawY,
		pos.x + sizeX,
		drawY + sizeY,
		barColor,
		TRUE
	);

	DrawBox(
		pos.x - 5,
		knobY - 8,
		pos.x + sizeX + 5,
		knobY + 8,
		knobColor,
		TRUE
	);
}

bool VerticalSlider::IsUsing() {
	return dragging;
}
#pragma endregion

KeyBindButton::KeyBindButton(Pos pos, int w, int h, int* keyPtr)
	: UI(pos, GetColor(150, 150, 255), w, h), targetKey(keyPtr) {}

void KeyBindButton::Update(int offsetY) {
	if (IsMouseOver(pos.x, pos.y , sizeX, sizeY, offsetY) &&
		Input::IsMouseTrigger(MOUSE_INPUT_LEFT)) {
		waitingInput = true;
		just = true;
	}

	if (waitingInput) {
		if (just) {
			just = false;
			return;
		}

		int key = Input::GetAnyKeyTrigger(); 

		if (key != -1) {
			if (!Input::IsValidBindKey(key)) {
				return;
			}

			*targetKey = key;
			waitingInput = false;
		}
	}
}

void KeyBindButton::Draw(int offsetY){
	int drawColor = isFocused
		? GetColor(255, 255, 0)
		: color;

	DrawBox(pos.x, pos.y - offsetY, pos.x + sizeX, pos.y + sizeY - offsetY,
		drawColor, TRUE);

	string text;

	if (waitingInput) {
		text = "Press Key...";
	}
	else {
		text = GetKeyName();
	}

	DrawString(
		pos.x + 10,
		pos.y + 10 - offsetY,
		text.c_str(),
		BLACK
	);
}

void KeyBindButton::OnClick() {
	waitingInput = true;
	just = true;
}

string KeyBindButton::GetKeyName() const {
	return KeyToStringSafe(*targetKey);
}

PadBindButton::PadBindButton(Pos pos, int w, int h, int* padPtr)
	: UI(pos, GetColor(150, 150, 255), w, h), targetPad(padPtr) {
}

void PadBindButton::Update(int offsetY) {
	if (IsMouseOver(pos.x, pos.y, sizeX, sizeY, offsetY) &&
		Input::IsMouseTrigger(MOUSE_INPUT_LEFT)) {
		waitingInput = true;
		just = true;
	}

	if (waitingInput) {
		if (just) {
			just = false;
			return;
		}

		int pad = Input::GetAnyPadTrigger();

		if (pad != -1) {
			*targetPad = pad;
			waitingInput = false;
		}
	}
}

void PadBindButton::Draw(int offsetY) {
	int drawColor = isFocused
		? GetColor(255, 255, 0)
		: color;

	DrawBox(pos.x, pos.y - offsetY, pos.x + sizeX, pos.y + sizeY - offsetY,
		drawColor, TRUE);

	string text = waitingInput ? "Press Pad..." : GetPadName();

	DrawString(pos.x + 10, pos.y + 10 - offsetY, text.c_str(), BLACK);
}

void PadBindButton::OnClick() {
	waitingInput = true;
	just = true;
}

string PadBindButton::GetPadName() const {
	return PadToStringSafe(*targetPad);
}

Label::Label(Pos pos, int sizeX, int sizeY, string text, int color, int fontSize) : UI(pos, color, sizeX, sizeY), text(text), fontSize(fontSize) {}

void Label::Update(int offsetY) {

}

void Label::Draw(int offsetY) {
	DrawStringToHandle(pos.x, pos.y - offsetY, text.c_str(), color, FontManager::Get(fontSize));
}
