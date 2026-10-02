#include "FontManager.h"
#include "DxLib.h"

std::map<int, int> FontManager::fonts;

void FontManager::Init() {

	// よく使うサイズを事前生成
	fonts[16] = CreateFontToHandle(NULL, 16, 3);
	fonts[24] = CreateFontToHandle(NULL, 24, 3);
	fonts[32] = CreateFontToHandle(NULL, 32, 3);
	fonts[48] = CreateFontToHandle(NULL, 48, 3);
	fonts[64] = CreateFontToHandle(NULL, 64, 3);
}

int FontManager::Get(int size) {

	// 無ければ自動生成
	if (fonts.count(size) == 0) {

		fonts[size] = CreateFontToHandle(
			NULL,
			size,
			3
		);
	}

	return fonts[size];
}

void FontManager::End() {

	for (auto& f : fonts) {
		DeleteFontToHandle(f.second);
	}

	fonts.clear();
}