#include"DxLib.h"
#include"Sound.h"
void SoundManager::Load(const string& name, const string& path, SoundType type) {
    int handle = LoadSoundMem(path.c_str());

    if (handle == -1)
        return;

    sounds[name] = { handle, type };
}

void SoundManager::Play(const string& name) {
    auto it = sounds.find(name);

    if (it == sounds.end())
        return;
    int volume;
    if (it->second.type == SoundType::BGM)
    {
        if (currentBGM != -1)
        {
            StopSoundMem(currentBGM);
        }

        currentBGM = it->second.handle;

        volume = 255 * masterVolume * bgmVolume / 10000;
        ChangeVolumeSoundMem(volume, currentBGM);
        PlaySoundMem(currentBGM, DX_PLAYTYPE_LOOP);
    }
    else
    {
        volume = 255 * masterVolume * seVolume / 10000;

        ChangeVolumeSoundMem(volume, it->second.handle);
        PlaySoundMem(it->second.handle, DX_PLAYTYPE_BACK);
    }
}

void SoundManager::StopBGM()
{
    if (currentBGM != -1)
    {
        StopSoundMem(currentBGM);
        currentBGM = -1;
    }
}

void SoundManager::SetVolume(const Config& config) {
    masterVolume = config.masterVolume;
    bgmVolume = config.BGMVolume;
    seVolume = config.SEVolume;

    int bgm = 255 * masterVolume * bgmVolume / 10000;

    ChangeVolumeSoundMem(bgm, currentBGM);
}

void SoundManager::Release()
{
    for (auto& sound : sounds)
    {
        DeleteSoundMem(sound.second.handle);
    }

    sounds.clear();
    currentBGM = -1;
}