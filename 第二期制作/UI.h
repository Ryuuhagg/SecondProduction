//UI.h
#pragma once
#include"Constant.h"
#include<functional>
#include<string>
using namespace std;
struct Pos {
	int x;
	int y;
};

class UI {
protected:
	Pos pos;
	int color;

	int sizeX;
	int sizeY;

	int groupId = -1;

	bool IsMouseOver(int x, int y, int w, int h, int offsetY);
	bool isFocused = false;
public:
	UI(Pos pos, int c, int sizeX, int sizeY) : pos(pos), color(c), sizeX(sizeX),sizeY(sizeY){}
	virtual void Update(int offsetY) = 0;
	virtual void Draw(int offsetY) = 0;

	virtual bool IsUsing() { return false; }
	virtual void OnClick(){}

	Pos GetPos() const { return pos; }
	void SetPos(const Pos& p) { pos = p; }

	void SetGroupId(int id) { groupId = id; }
	int GetGroupId() const { return groupId; }

	void SetFocus(bool f) { isFocused = f; }
	bool IsFocused() const { return isFocused; }

	virtual bool IsSelectable() { return true; }

	bool IsMouseOverSelf(int offsetY) {
		return IsMouseOver(pos.x, pos.y, sizeX, sizeY, offsetY);
	}
};

class Button : public UI {
	bool clicked;

	function<void()> onClick;
public:
	Button(Pos pos, int sizeX, int sizeY);
	void Update(int offsetY) override;
	void Draw(int offsetY) override;
	bool IsClicked(int offsetY);

	bool IsUsing()override;

	void SetOnClick(function<void()> func);

	void OnClick()override;
};

class Toggle : public UI {
	bool isOn = false;
	bool usingNow = false;

	string label;

	function<void()> onClick;
public:
	Toggle(Pos pos, int sizeX, int sizeY);
	void Update(int offsetY) override;
	void Draw(int offsetY) override;

	void SetOnClick(function<void()> func);

	void OnClick()override;

	bool IsClicked(int offsetY);
	bool IsUsing()override;

	void Set(bool v) { isOn = v; }
	bool Get() const { return isOn; }
};

class Slider : public UI {
	float value;
	bool dragging;
public:
	Slider(Pos pos, int w, int h);

	void Update(int offsetY) override;
	void Draw(int offsetY) override;

	float GetValue() const { return value; }

	bool IsUsing()override;

	void SetValue(float v) {
		value = v;
		if (value < 0.0f) value = 0.0f;
		if (value > 1.0f) value = 1.0f;
	}
};

class VerticalSlider : public UI {
	float value;
	bool dragging;
public:
	VerticalSlider(Pos pos, int w, int h);

	void Update(int offsetY) override;
	void Draw(int offsetY) override;

	float GetValue() const { return value; }

	bool IsUsing()override;

	void SetValue(float v) {
		value = v;
		if (value < 0.0f) value = 0.0f;
		if (value > 1.0f) value = 1.0f;
	}
};

class KeyBindButton : public UI {
	bool waitingInput = false;
	bool just = false;
	int* targetKey; // Configのキーを直接指す

public:
	KeyBindButton(Pos pos, int w, int h, int* keyPtr);

	void Update(int offsetY) override;

	void Draw(int offsetY)  override;

	void OnClick()override;

	bool IsUsing() override { return waitingInput; }

	string GetKeyName() const;
};

class PadBindButton : public UI {
	bool waitingInput = false;
	bool just = false;
	int* targetPad; 

public:
	PadBindButton(Pos pos, int w, int h, int* padPtr);

	void Update(int offsetY) override;

	void Draw(int offsetY)  override;

	void OnClick()override;

	bool IsUsing() override { return waitingInput; }

	string GetPadName() const;
};

class Label : public UI {
	string text;
	int fontSize;
public:
	Label(Pos pos, int sizeX, int sizeY, string text, int color, int fontSize = 16);
	void Update(int offsetY)override;
	void Draw(int offsetY) override;

	bool IsSelectable() override { return false; }
};
