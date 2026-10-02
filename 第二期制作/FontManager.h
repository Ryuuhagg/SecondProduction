#pragma once
#include <map>

class FontManager {
private:
	static std::map<int, int> fonts;

public:
	static void Init();
	static int Get(int size);
	static void End();
};