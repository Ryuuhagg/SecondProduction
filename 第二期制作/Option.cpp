//Option.cpp
#include <unordered_map>
#include <algorithm>
#include <string>
#include"Option.h"
#include"DxLib.h"
#include<fstream>
#include"json.hpp"

using namespace std;
using json = nlohmann::json;

string ToUpper(string s) {
	transform(s.begin(), s.end(), s.begin(), ::toupper);
	return s;
}

#pragma region Key
unordered_map<string, int> stringToKey = {
	{"SPACE", KEY_INPUT_SPACE},
	{"ENTER", KEY_INPUT_RETURN},
	{"ESC", KEY_INPUT_ESCAPE},

	{"A", KEY_INPUT_A},
	{"B", KEY_INPUT_B},
	{"C", KEY_INPUT_C},
	{"D", KEY_INPUT_D},
	{"E", KEY_INPUT_E},
	{"F", KEY_INPUT_F},
	{"G", KEY_INPUT_G},
	{"H", KEY_INPUT_H},
	{"I", KEY_INPUT_I},
	{"J", KEY_INPUT_J},
	{"K", KEY_INPUT_K},
	{"L", KEY_INPUT_L},
	{"M", KEY_INPUT_M},
	{"N", KEY_INPUT_N},
	{"O", KEY_INPUT_O},
	{"P", KEY_INPUT_P},
	{"Q", KEY_INPUT_Q},
	{"R", KEY_INPUT_R},
	{"S", KEY_INPUT_S},
	{"T", KEY_INPUT_T},
	{"U", KEY_INPUT_U},
	{"V", KEY_INPUT_V},
	{"W", KEY_INPUT_W},
	{"X", KEY_INPUT_X},
	{"Y", KEY_INPUT_Y},
	{"Z", KEY_INPUT_Z},

	{"UP", KEY_INPUT_UP},
	{"DOWN", KEY_INPUT_DOWN},
	{"LEFT", KEY_INPUT_LEFT},
	{"RIGHT", KEY_INPUT_RIGHT},

	{"LSHIFT", KEY_INPUT_LSHIFT}
};

unordered_map<int, string> keyToString = {
	{KEY_INPUT_SPACE, "SPACE"},
	{KEY_INPUT_RETURN, "ENTER"},
	{KEY_INPUT_ESCAPE, "ESC"},

	{KEY_INPUT_A, "A"},
	{KEY_INPUT_B, "B"},
	{KEY_INPUT_C, "C"},
	{KEY_INPUT_D, "D"},
	{KEY_INPUT_E, "E"},
	{KEY_INPUT_F, "F"},
	{KEY_INPUT_G, "G"},
	{KEY_INPUT_H, "H"},
	{KEY_INPUT_I, "I"},
	{KEY_INPUT_J, "J"},
	{KEY_INPUT_K, "K"},
	{KEY_INPUT_L, "L"},
	{KEY_INPUT_M, "M"},
	{KEY_INPUT_N, "N"},
	{KEY_INPUT_O, "O"},
	{KEY_INPUT_P, "P"},
	{KEY_INPUT_Q, "Q"},
	{KEY_INPUT_R, "R"},
	{KEY_INPUT_S, "S"},
	{KEY_INPUT_T, "T"},
	{KEY_INPUT_U, "U"},
	{KEY_INPUT_V, "V"},
	{KEY_INPUT_W, "W"},
	{KEY_INPUT_X, "X"},
	{KEY_INPUT_Y, "Y"},
	{KEY_INPUT_Z, "Z"},

	{KEY_INPUT_UP, "UP"},
	{KEY_INPUT_DOWN, "DOWN"},
	{KEY_INPUT_LEFT, "LEFT"},
	{KEY_INPUT_RIGHT, "RIGHT"},

	{KEY_INPUT_LSHIFT, "LSHIFT"}
};

int StringToKeySafe(const string& s) {
	string upper = ToUpper(s);
	return stringToKey.count(upper) ? stringToKey[upper] : KEY_INPUT_SPACE;
}

string KeyToStringSafe(int key) {
	return keyToString.count(key) ? keyToString[key] : "UNKNOWN";
}
#pragma endregion
#pragma region Pad
unordered_map<string, int> stringToPad = {
	{"A", PAD_INPUT_1},
	{"B", PAD_INPUT_2},
	{"X", PAD_INPUT_3},
	{"Y", PAD_INPUT_4},
	{"L", PAD_INPUT_5},
	{"R", PAD_INPUT_6},
	{"BACK", PAD_INPUT_7},
	{"START", PAD_INPUT_8},
	{"PSL", PAD_INPUT_9},
	{"PSR", PAD_INPUT_10}
};

unordered_map<int, string> PadToString = {
	{PAD_INPUT_1, "A"},
	{PAD_INPUT_2, "B"},
	{PAD_INPUT_3, "X"},
	{PAD_INPUT_4, "Y"},
	{PAD_INPUT_5, "L"},
	{PAD_INPUT_6, "R"},
	{PAD_INPUT_7, "BACK"},
	{PAD_INPUT_8, "START"},
	{PAD_INPUT_9, "PSL"},
	{PAD_INPUT_10, "PSR"}
};

int StringToPadSafe(const string& s) {
	string upper = ToUpper(s);
	return stringToPad.count(upper) ? stringToPad[upper] : PAD_INPUT_1;
}

string PadToStringSafe(int pad) {
	return PadToString.count(pad) ? PadToString[pad] : "UNKNOWN";
}
#pragma endregion


void Option::SaveOption(const Config& config) {
	json j;

	j["masterVolume"] = config.masterVolume;
	j["SEVolume"] = config.SEVolume;
	j["BGMVolume"] = config.BGMVolume;

	j["CamelaUpDownFlip"] = config.CamelaUpDownFlip;
	j["CamelaRightLeftFlip"] = config.CamelaRightLeftFlip;

	j["KeyJump"] = KeyToStringSafe(config.KeyJump);
	j["PadJump"] = PadToStringSafe(config.PadJump);

	j["KeyConfirm"] = KeyToStringSafe(config.KeyConfirm);
	j["PadConfirm"] = PadToStringSafe(config.PadConfirm);

	j["KeyCancel"] = KeyToStringSafe(config.KeyCancel);
	j["PadCancel"] = PadToStringSafe(config.PadCancel);

	j["KeyDash"] = KeyToStringSafe(config.KeyDash);
	j["PadDash"] = PadToStringSafe(config.PadDash);

	j["KeyPause"] = KeyToStringSafe(config.KeyPause);
	j["PadPause"] = PadToStringSafe(config.PadPause);

	j["mouseSensitivity"] = config.mouseSensitivity;
	j["padSensitivity"] = config.padSensitivity;

	j["KeyFront"] = KeyToStringSafe(config.KeyFront);
	j["KeyBack"] = KeyToStringSafe(config.KeyBack);
	j["KeyRight"] = KeyToStringSafe(config.KeyRight);
	j["KeyLeft"] = KeyToStringSafe(config.KeyLeft);

	ofstream file("config.json");
	if (file) {
		file << j.dump(4);
	}
}

void Option::LoadOption(Config& config) {
	ifstream file("config.json");

	if (file) {
		json j;
		file >> j;

		config.masterVolume = j.value("masterVolume", 50);
		config.SEVolume = j.value("SEVolume", 50);
		config.BGMVolume = j.value("BGMVolume", 50);

		config.CamelaUpDownFlip = j.value("CamelaUpDownFlip", false);
		config.CamelaRightLeftFlip = j.value("CamelaRightLeftFlip", false);

		config.KeyJump = StringToKeySafe(j.value("KeyJump", "SPACE"));
		config.PadJump = StringToPadSafe(j.value("PadJump", "X"));

		config.KeyConfirm = StringToKeySafe(j.value("KeyConfirm", "ENTER"));
		config.PadConfirm = StringToPadSafe(j.value("PadConfirm", "B"));

		config.KeyCancel = StringToKeySafe(j.value("KeyCancel", "ESC"));
		config.PadCancel = StringToPadSafe(j.value("PadCancel", "A"));

		config.KeyDash = StringToKeySafe(j.value("KeyDash", "LSHIFT"));
		config.PadDash = StringToPadSafe(j.value("PadDash", "PSL"));

		config.KeyPause = StringToKeySafe(j.value("KeyPause", "ESC"));
		config.PadPause = StringToPadSafe(j.value("PadPause", "START"));

		config.mouseSensitivity = j.value("mouseSensitivity", 0.005f);
		config.padSensitivity = j.value("padSensitivity", 0.03f);

		config.KeyFront = StringToKeySafe(j.value("KeyFront", "W"));
		config.KeyBack = StringToKeySafe(j.value("KeyBack", "S"));
		config.KeyRight = StringToKeySafe(j.value("KeyRight", "D"));
		config.KeyLeft = StringToKeySafe(j.value("KeyLeft", "A"));
	}
	else {
		config.masterVolume = 50;
		config.SEVolume = 50;
		config.BGMVolume = 50;

		config.CamelaUpDownFlip = false;
		config.CamelaRightLeftFlip = false;

		config.KeyJump = KEY_INPUT_SPACE;
		config.PadJump = PAD_INPUT_3;

		config.KeyConfirm = KEY_INPUT_RETURN;
		config.PadConfirm = PAD_INPUT_2;

		config.KeyCancel = KEY_INPUT_ESCAPE;
		config.PadCancel = PAD_INPUT_1;

		config.KeyDash = KEY_INPUT_LSHIFT;
		config.PadDash = PAD_INPUT_9;

		config.KeyPause = KEY_INPUT_ESCAPE;
		config.PadPause = PAD_INPUT_8;

		config.mouseSensitivity = 0.005f;
		config.padSensitivity = 0.03f;

		config.KeyFront = KEY_INPUT_W;
		config.KeyBack = KEY_INPUT_S;
		config.KeyRight = KEY_INPUT_D;
		config.KeyLeft = KEY_INPUT_A;

		SaveOption(config);
	}
}
