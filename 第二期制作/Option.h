//Option.h
#pragma once
#include <string>
//キー設定
struct Config {
	int masterVolume;
	int SEVolume;
	int BGMVolume;

	bool CamelaUpDownFlip;
	bool CamelaRightLeftFlip;

	int KeyJump;
	int PadJump;

	int KeyConfirm;
	int PadConfirm;

	int KeyCancel;
	int PadCancel;

	int KeyDash;
	int PadDash;

	int KeyPause;
	int PadPause;
	//マウスとパッドのカメラ操作の感度
	float mouseSensitivity;
	float padSensitivity;

	int KeyFront;
	int KeyBack;
	int KeyRight;
	int KeyLeft;
};

class Option {
	Config config;
public:
	void SaveOption(const Config& config);
	void LoadOption(Config& config);
};

int StringToKeySafe(const std::string& s);
std::string KeyToStringSafe(int key);

int StringToPadSafe(const std::string& s);
std::string PadToStringSafe(int pad);