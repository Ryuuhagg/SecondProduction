#pragma once
#include<string>
#include<unordered_map>
#include"Option.h"


using namespace std;

enum class SoundType
{
	BGM,
	SE
};

struct SoundData
{
	int handle;
	SoundType type;
};

class SoundManager {
	unordered_map<string, SoundData> sounds;

	int currentBGM = -1;

	int masterVolume;
	int bgmVolume;
	int seVolume;
public:
	~SoundManager() { Release(); }
	void Load(const string& name, const string& path, SoundType type);

	void Play(const string& name);

	void StopBGM();

	void SetVolume(const Config& config);

	void Release();
};