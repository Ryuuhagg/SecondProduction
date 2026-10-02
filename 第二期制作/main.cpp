//main.cpp
//メイン処理書く場所
//ヒロシです。。。
#include"DxLib.h"
#include"SceneManager.h"
#include"Constant.h"
#include"Input.h"
using namespace std;
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

	SetGraphMode(WIDTH, HEIGHT, 32);
	ChangeWindowMode(TRUE);
	SetMainWindowText("迷路");
	SetBackgroundColor(80, 120, 160);
	if (DxLib_Init() == -1) return -1;
	//SetUseLighting(FALSE);
	//SetUseBackCulling(FALSE);
	SetUseZBuffer3D(TRUE);
	SetWriteZBuffer3D(TRUE);
	SetDrawScreen(DX_SCREEN_BACK);
	SceneManager manager;
	Input input;
	manager.ChangeScene(make_unique<Title>());

	manager.GetSoundManager().Load("bgm0", "Sound/BGM1.wav", SoundType::BGM);
	manager.GetSoundManager().Load("bgm1", "Sound/maou_bgm_orchestra20.mp3", SoundType::BGM);
	manager.GetSoundManager().Load("bgm2", "Sound/maou_bgm_orchestra21.mp3", SoundType::BGM);
	manager.GetSoundManager().Load("bgm3", "Sound/maou_bgm_orchestra25.mp3", SoundType::BGM);
	manager.GetSoundManager().Load("bgm4", "Sound/maou_bgm_piano33.mp3", SoundType::BGM);
	manager.GetSoundManager().Load("save", "Sound/save.mp3", SoundType::SE);
	manager.GetSoundManager().Load("cancel", "Sound/cancel.mp3", SoundType::SE);
	manager.GetSoundManager().Load("cursor_move", "Sound/cursor_move.mp3", SoundType::SE);
	manager.GetSoundManager().Load("confirm", "Sound/confirm.mp3", SoundType::SE);

	SetLightDirection(VGet(0.0f, -1.0f, 0.5f));

	SetLightDifColor(GetColorF(1.0f, 1.0f, 1.0f,255));
	SetLightSpcColor(GetColorF(1.0f, 1.0f, 1.0f,255));

	SetGlobalAmbientLight(GetColorF(0.8f, 0.8f, 0.8f,255));

	while (ProcessMessage() == 0 ) {
		ClearDrawScreen();

		input.Update();
		manager.Update();
		manager.Draw();

		ScreenFlip();
		WaitTimer(16);
	}
	DxLib_End();
	return 0;
}