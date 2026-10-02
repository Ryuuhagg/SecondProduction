#include"OptionMenuUI.h"
#include"Input.h"
#include"DxLib.h"
#include"Constant.h"

void OptionMenuUI::Init() {
	backRequested = false;
	rebuildUI = false;
	time = 0;

	option.LoadOption(config);
	BuildUI();
}

void OptionMenuUI::Update() {
	if (time > 0) {
		time--;
	}

	uiManager.Update();

	if (rebuildUI) {
		rebuildUI = false;
		BuildUI();
	}
}

void OptionMenuUI::Draw() {
	uiManager.Draw();

	if (time > 0) {
		DrawString(WIDTH / 2, HEIGHT / 2, "保存されました", WHITE);
	}
}

bool OptionMenuUI::IsBackRequested() const {
	return backRequested;
}

void OptionMenuUI::ResetBackRequest() {
	backRequested = false;
}

void OptionMenuUI::SaveCurrentConfig() {
	if (master) config.masterVolume = (int)(master->GetValue() * 100);
	if (bgm) config.BGMVolume = (int)(bgm->GetValue() * 100);
	if (se) config.SEVolume = (int)(se->GetValue() * 100);

	config.mouseSensitivity = mouseSensitivity->GetValue() * 0.01f;
	config.padSensitivity = padSensitivity->GetValue() * 0.1f;

	if (invertY) config.CamelaUpDownFlip = invertY->Get();
	if (invertX) config.CamelaRightLeftFlip = invertX->Get();

	option.SaveOption(config);
	Input::SetConfig(config);

	soundManager->SetVolume(config);

	time = 100;
}

void OptionMenuUI::BuildUI() {
	uiManager.Clear();
	uiManager.SetScrollEnabled(false);

	int x = 300;
	int y = 100;
	int line = 60;

	auto title = make_shared<Label>(Pos{ 100, 60 }, 0, 0, "OPTION", WHITE, 48);

	auto commonBtn = make_shared<Button>(Pos{ 260, 140 }, 180, 45);
	commonBtn->SetOnClick([this]() {
		currentTab = CommonTab;
		rebuildUI = true;
		Play("confirm");
		});

	auto keyboardBtn = make_shared<Button>(Pos{ 460, 140 }, 180, 45);
	keyboardBtn->SetOnClick([this]() {
		currentTab = KeyboardTab;
		rebuildUI = true;
		Play("confirm");
		});

	auto padBtn = make_shared<Button>(Pos{ 660, 140 }, 180, 45);
	padBtn->SetOnClick([this]() {
		currentTab = PadTab;
		rebuildUI = true;
		Play("confirm");
		});

	commonBtn->SetGroupId(1);
	keyboardBtn->SetGroupId(1);
	padBtn->SetGroupId(1);

	uiManager.Add(title);

	uiManager.Add(commonBtn);
	uiManager.Add(make_shared<Label>(Pos{ 315, 154 }, 0, 0, "基本設定", BLACK));

	uiManager.Add(keyboardBtn);
	uiManager.Add(make_shared<Label>(Pos{ 515, 154 }, 0, 0, "キー設定", BLACK));

	uiManager.Add(padBtn);
	uiManager.Add(make_shared<Label>(Pos{ 725, 154 }, 0, 0, "パッド設定", BLACK));

	y = 230;

	if (currentTab == CommonTab) {
		uiManager.Add(make_shared<Label>(Pos{ x, y }, 0, 0, "Master Volume", WHITE));
		master = make_shared<Slider>(Pos{ x + 220, y }, 300, 20);
		master->SetValue(config.masterVolume / 100.0f);
		uiManager.Add(master);
		y += line;

		uiManager.Add(make_shared<Label>(Pos{ x, y }, 0, 0, "BGM", WHITE));
		bgm = make_shared<Slider>(Pos{ x + 220, y }, 300, 20);
		bgm->SetValue(config.BGMVolume / 100.0f);
		uiManager.Add(bgm);
		y += line;

		uiManager.Add(make_shared<Label>(Pos{ x, y }, 0, 0, "SE", WHITE));
		se = make_shared<Slider>(Pos{ x + 220, y }, 300, 20);
		se->SetValue(config.SEVolume / 100.0f);
		uiManager.Add(se);
		y += line;

		uiManager.Add(make_shared<Label>(Pos{ x, y }, 0, 0, "マウス感度", WHITE));
		mouseSensitivity = make_shared<Slider>(Pos{ x + 220, y }, 300, 20);
		mouseSensitivity->SetValue(config.mouseSensitivity / 0.01f);
		uiManager.Add(mouseSensitivity);
		y += line;

		uiManager.Add(make_shared<Label>(Pos{ x, y }, 0, 0, "パッド感度", WHITE));
		padSensitivity = make_shared<Slider>(Pos{ x + 220, y }, 300, 20);
		padSensitivity->SetValue(config.padSensitivity / 0.1f);
		uiManager.Add(padSensitivity);
		y += line;


		uiManager.Add(make_shared<Label>(Pos{ x, y }, 0, 0, "カメラ上下反転", WHITE));
		invertY = make_shared<Toggle>(Pos{ x + 220, y }, 50, 30);
		invertY->Set(config.CamelaUpDownFlip);
		invertY->SetOnClick([this]() {
			Play("confirm");
			});
		uiManager.Add(invertY);
		y += line;

		uiManager.Add(make_shared<Label>(Pos{ x, y }, 0, 0, "カメラ左右反転", WHITE));
		invertX = make_shared<Toggle>(Pos{ x + 220, y }, 50, 30);
		invertX->Set(config.CamelaRightLeftFlip);
		invertX->SetOnClick([this]() {
			Play("confirm");
			});
		uiManager.Add(invertX);
		y += line;

		master->SetGroupId(2);
		bgm->SetGroupId(3);
		se->SetGroupId(4);
		mouseSensitivity->SetGroupId(5);
		padSensitivity->SetGroupId(6);
		invertY->SetGroupId(7);
		invertX->SetGroupId(8);
	}
	else if (currentTab == KeyboardTab) {
		auto jumpBtn = make_shared<KeyBindButton>(Pos{ x + 220, y }, 150, 40, &config.KeyJump);
		jumpBtn->SetGroupId(2);
		uiManager.Add(make_shared<Label>(Pos{ x, y }, 0, 0, "ジャンプキー", WHITE));
		uiManager.Add(jumpBtn);
		y += line;

		auto dashBtn = make_shared<KeyBindButton>(Pos{ x + 220, y }, 150, 40, &config.KeyDash);
		dashBtn->SetGroupId(3);
		uiManager.Add(make_shared<Label>(Pos{ x, y }, 0, 0, "ダッシュキー", WHITE));
		uiManager.Add(dashBtn);
		y += line;

		auto confirmBtn = make_shared<KeyBindButton>(Pos{ x + 220, y }, 150, 40, &config.KeyConfirm);
		confirmBtn->SetGroupId(4);
		uiManager.Add(make_shared<Label>(Pos{ x, y }, 0, 0, "決定キー", WHITE));
		uiManager.Add(confirmBtn);
		y += line;

		auto cancelBtn = make_shared<KeyBindButton>(Pos{ x + 220, y }, 150, 40, &config.KeyCancel);
		cancelBtn->SetGroupId(5);
		uiManager.Add(make_shared<Label>(Pos{ x, y }, 0, 0, "キャンセルキー", WHITE));
		uiManager.Add(cancelBtn);
		y += line;

		auto pauseBtn = make_shared<KeyBindButton>(Pos{ x + 220, y }, 150, 40, &config.KeyPause);
		pauseBtn->SetGroupId(6);
		uiManager.Add(make_shared<Label>(Pos{ x, y }, 0, 0, "ポーズキー", WHITE));
		uiManager.Add(pauseBtn);
		y += line;
	}
	else if (currentTab == PadTab) {
		auto jumpBtn = make_shared<PadBindButton>(Pos{ x + 220, y }, 150, 40, &config.PadJump);
		jumpBtn->SetGroupId(2);
		uiManager.Add(make_shared<Label>(Pos{ x, y }, 0, 0, "ジャンプボタン", WHITE));
		uiManager.Add(jumpBtn);
		y += line;

		auto dashBtn = make_shared<PadBindButton>(Pos{ x + 220, y }, 150, 40, &config.PadDash);
		dashBtn->SetGroupId(3);
		uiManager.Add(make_shared<Label>(Pos{ x, y }, 0, 0, "ダッシュボタン", WHITE));
		uiManager.Add(dashBtn);
		y += line;

		auto confirmBtn = make_shared<PadBindButton>(Pos{ x + 220, y }, 150, 40, &config.PadConfirm);
		confirmBtn->SetGroupId(4);
		uiManager.Add(make_shared<Label>(Pos{ x, y }, 0, 0, "決定ボタン", WHITE));
		uiManager.Add(confirmBtn);
		y += line;

		auto cancelBtn = make_shared<PadBindButton>(Pos{ x + 220, y }, 150, 40, &config.PadCancel);
		cancelBtn->SetGroupId(5);
		uiManager.Add(make_shared<Label>(Pos{ x, y }, 0, 0, "キャンセルボタン", WHITE));
		uiManager.Add(cancelBtn);
		y += line;

		auto pauseBtn = make_shared<PadBindButton>(Pos{ x + 220, y }, 150, 40, &config.PadPause);
		pauseBtn->SetGroupId(6);
		uiManager.Add(make_shared<Label>(Pos{ x, y }, 0, 0, "ポーズボタン", WHITE));
		uiManager.Add(pauseBtn);
		y += line;
	}
	auto saveBtn = make_shared<Button>(Pos{ x + 100, y }, 150, 50);
	saveBtn->SetOnClick([this]() {
		SaveCurrentConfig();
		Play("save");
		});

	auto backBtn = make_shared<Button>(Pos{ x + 300, y }, 150, 50);
	backBtn->SetOnClick([this]() {
		backRequested = true;
		Play("cancel");
		});

	saveBtn->SetGroupId(20);
	backBtn->SetGroupId(20);

	uiManager.Add(saveBtn);
	uiManager.Add(make_shared<Label>(Pos{ x + 150, y + 18 }, 0, 0, "保存", BLACK));

	uiManager.Add(backBtn);
	uiManager.Add(make_shared<Label>(Pos{ x + 350, y + 18 }, 0, 0, "戻る", BLACK));
}

void OptionMenuUI::Play(const string& name) {
	if (soundManager) {
		soundManager->Play(name);
	}
}

void OptionMenuUI::SetSoundManager(SoundManager* sound) {
	soundManager = sound;
}