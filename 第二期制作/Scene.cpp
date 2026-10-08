//Scene.cpp
#include"Scene.h"
#include"Constant.h"
#include"SceneManager.h"
#include"MapLoader.h"
#include"Character.h"
#include"Input.h"
#include"UI.h"
#include"UIManager.h"
#include"DxLib.h"
#include"MapEditor.h"
#include"Image.h"
#include"FontManager.h"
// 2026-06-02: 取るもの/避けるもの/鍵/投げ敵をゲームシーンで動かすため追加。
#include"GameObjects.h"
// 2026-06-12: ?????G??EVENT???番?????A?G??p?????????????????????B
#include"GameEnemies.h"
#include"Player.h"
#include"Camera.h"
#include"EnemyManager.h"
#include<fstream>
#include<vector>
#include<string>
#include<cstring>
#include<windows.h>
using namespace std;
Player p;
Camera c(p);
EnemyManager e;
static bool IsMapCsvExists(int mapIndex)
{
    if (mapIndex <= 0)
        return false;

    char fileName[260];
    if (gameCurrentMapName[0] != '\0')
        sprintf_s(fileName, sizeof(fileName), "maps\\%s\\map.csv", gameCurrentMapName);
    else
        sprintf_s(fileName, sizeof(fileName), "maps\\map%d\\map.csv", mapIndex);

    ifstream ifs(fileName);
    return ifs.good();
}

static vector<string> titleMapNames;

static void RefreshTitleMapNames();

static int FindTitleMapNameIndex(const char* mapName);

static void MoveTitleMapSelection(
	int& selectedMapIndex,
	int direction
);

static const char* GetSelectedTitleMapName(
	int selectedMapIndex
);

static bool IsEditorMapStartGoalOnFloor();

static int FindTitleMapNameIndex(const char* mapName)
{
	for (int i = 0; i < (int)titleMapNames.size(); i++)
	{
		if (titleMapNames[i] == mapName)
			return i;
	}

	return 0;
}

static bool IsTitleMapConfigCsv(const char* name)
{
	return strcmp(name, "model_config") == 0 || strcmp(name, "collision_config") == 0;
}

static string GetCsvBaseName(const char* fileName)
{
	string name = fileName;
	size_t dot = name.find_last_of('.');
	if (dot != string::npos)
		name = name.substr(0, dot);
	return name;
}



static void RefreshTitleMapNames()
{
    // 2026-07-15: タイトルも maps/マップ名/map.csv を持つフォルダからマップ一覧を作る。
    titleMapNames.clear();
    CreateDirectoryA("maps", NULL);

    WIN32_FIND_DATAA findData;
    HANDLE handle = FindFirstFileA("maps\\*", &findData);
    if (handle != INVALID_HANDLE_VALUE)
    {
        do
        {
            if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
                continue;
            if (strcmp(findData.cFileName, ".") == 0 || strcmp(findData.cFileName, "..") == 0)
                continue;

            char csvPath[260];
            sprintf_s(csvPath, sizeof(csvPath), "maps\\%s\\map.csv", findData.cFileName);
            ifstream mapCsv(csvPath);
            if (!mapCsv)
                continue;

            titleMapNames.push_back(findData.cFileName);
        } while (FindNextFileA(handle, &findData));

        FindClose(handle);
    }

    if (titleMapNames.empty())
        titleMapNames.push_back("map1");
}



static int FindSelectableMapIndex(int currentIndex, int direction)
{
	// 2026-05-18: map1.csv, map2.csv のように増やしたCSVだけをタイトル画面で選べるようにする。
	const int maxMapIndex = 99;

	for (int i = 1; i <= maxMapIndex; i++)
	{
		int nextIndex = currentIndex + direction * i;

		if (nextIndex < 1)
			nextIndex = maxMapIndex + nextIndex;

		if (nextIndex > maxMapIndex)
			nextIndex = nextIndex - maxMapIndex;

		if (IsMapCsvExists(nextIndex))
			return nextIndex;
	}

	return currentIndex;
}

static const char* GetSelectedTitleMapName(int selectedMapIndex)
{
	if (titleMapNames.empty())
		RefreshTitleMapNames();

	if (selectedMapIndex < 0 || selectedMapIndex >= (int)titleMapNames.size())
		return "map1";

	return titleMapNames[selectedMapIndex].c_str();
}


static bool IsEditorMapPosInRange(int layer, int z, int x)
{
	return layer >= 0 && layer < BLOCK_NUM_Y && z >= 0 && z < BLOCK_NUM_Z && x >= 0 && x < BLOCK_NUM_X;
}
static bool IsEditorMapStartGoalOnFloor()
{
	// 2026-07-16: 座標の初期値ではなく、START/GOALイベントが実際に置かれているかを先に確認する。
	if (!SyncEditorStartGoalFromEvents())
		return false;

	return IsEditorMapPosInRange(startY, startZ, startX) &&
		IsEditorMapPosInRange(goalY, goalZ, goalX) &&
		FloorMap[startY][startZ][startX] >= 0 &&
		FloorMap[goalY][goalZ][goalX] >= 0;
}
/*
static bool IsStartGoalConnected()
{
	LoadGameMap();
	if (!SyncEditorStartGoalFromEvents())
		return false;

	//if (startY != goalY)
		//return false;

	return !path.empty();
}
*/
Title::Title(){
	Init();
}
void Title::UISet() {
	uiManager.SetScrollEnabled(false);
	uiManager.Clear();
	auto title_text = make_shared<Label>(Pos{ 100, 100 }, 0, 0, "TITLE", WHITE, 32);

	auto gameBtn = make_shared<Button>(Pos{ 1000, 400 }, 150, 50);
	gameBtn->SetOnClick([this]() {
		changeFlg = 1;
		});

	auto game_text = make_shared<Label>(Pos{ 1050, 420 }, 0, 0, "遊ぶ", BLACK);

	auto optionBtn = make_shared<Button>(Pos{ 1000, 500 }, 150, 50);
	optionBtn->SetOnClick([this]() {
		changeFlg = 2;
		});

	auto option_text = make_shared<Label>(Pos{ 1050, 520 }, 0, 0, "設定", BLACK);

	auto endBtn = make_shared<Button>(Pos{ 1000, 600 }, 150, 50);
	endBtn->SetOnClick([this]() {
		DxLib_End();
		});

	auto end_text = make_shared<Label>(Pos{ 1050, 620 }, 0, 0, "終了", BLACK);

	gameBtn->SetGroupId(1);

	optionBtn->SetGroupId(2);

	endBtn->SetGroupId(3);

	//UIMAnagerに追加
	uiManager.Add(title_text);

	uiManager.Add(gameBtn);
	uiManager.Add(game_text);

	uiManager.Add(optionBtn);
	uiManager.Add(option_text);

	uiManager.Add(endBtn);
	uiManager.Add(end_text);
}
void Title::Init() {
	img = LoadGraph(title_image.path);
	SetMouseDispFlag(TRUE);

	changeFlg = 0;
	first = false;
	
	UISet();
}
static void MoveTitleMapSelection(int& selectedMapIndex, int direction)
{
	if (titleMapNames.empty())
		RefreshTitleMapNames();

	if (titleMapNames.empty())
		return;

	selectedMapIndex += direction;
	if (selectedMapIndex < 0)
		selectedMapIndex = (int)titleMapNames.size() - 1;
	if (selectedMapIndex >= (int)titleMapNames.size())
		selectedMapIndex = 0;
}
void Title::Draw() {
	DrawExtendGraph(0, 0, WIDTH, HEIGHT, img, true);
	
	uiManager.Draw();
}
void Title::Update(SceneManager& manager) {
	
	if (!first) {
		option.LoadOption(config);
		manager.GetSoundManager().SetVolume(config);
		Input::SetConfig(config);
		first = true;

		uiManager.SetSoundManager(&manager.GetSoundManager());
		manager.GetSoundManager().Play("bgm1");
	}

	//Padのボタンを調べるやつ
	/*
	DINPUT_JOYSTATE js;

	if (GetJoypadDirectInputState(DX_INPUT_PAD1, &js) == 0)
	{
		for (int i = 0; i < 32; i++)
		{
			if (js.Buttons[i])
			{
				printfDx("Button[%d] が押された\n", i);
			}
		}
	}
	if (Input::IsPadTrigger(PAD_INPUT_7)) {
		printfDx("aaa");
	}
	*/
	//Padを振動させるやつ
	//StartJoypadVibration(DX_INPUT_PAD1, 1000, 2000, -1);

	switch (changeFlg) {
	case 1:
		manager.GetSoundManager().Play("confirm");
		manager.ChangeScene(
			make_unique<Select>(),
			make_unique<Fade>()
		);
		break;
	case 2:
		manager.GetSoundManager().Play("confirm");
		manager.ChangeScene(
			make_unique<OptionMenu>(),
			make_unique<Slide>()
		);
		break;
	}
	uiManager.Update();
}

Select::Select() {
	Init();
}

Select::~Select()
{
	if (thumbnail_img != -1)
	{
		DeleteGraph(thumbnail_img);
		thumbnail_img = -1;
	}
}

void Select::ApplySelectedMap()
{
	const char* selectedMapName =
		GetSelectedTitleMapName(selectedMapIndex);

	strcpy_s(
		gameCurrentMapName,
		sizeof(gameCurrentMapName),
		selectedMapName
	);

	gameCurrentMapIndex = selectedMapIndex + 1;
}

void Select::LoadSelectedThumbnail()
{
	// 前のサムネイルを削除
	if (thumbnail_img != -1)
	{
		DeleteGraph(thumbnail_img);
		thumbnail_img = -1;
	}

	const char* selectedMapName =
		GetSelectedTitleMapName(selectedMapIndex);

	MapInfo mapInfo;

	if (!LoadMapInfoByName(selectedMapName, mapInfo))
	{
		return;
	}

	// サムネイルが登録されていない
	if (mapInfo.thumbnail[0] == '\0')
	{
		return;
	}

	thumbnail_img = LoadGraph(mapInfo.thumbnail);
}

void Select::UISet()
{
	uiManager.SetScrollEnabled(false);
	uiManager.Clear();

	// 前のマップ
	auto prevBtn = make_shared<Button>(
		Pos{ 170, 120 },
		160,
		55
	);

	prevBtn->SetOnClick([this]() {
		MoveTitleMapSelection(selectedMapIndex, -1);
		LoadSelectedThumbnail();
		});

	auto prevText = make_shared<Label>(
		Pos{ 215, 138 },
		0, 0,
		"前へ",
		BLACK
	);

	// 次のマップ
	auto nextBtn = make_shared<Button>(
		Pos{ 950, 120 },
		160,
		55
	);

	nextBtn->SetOnClick([this]() {
		MoveTitleMapSelection(selectedMapIndex, 1);
		LoadSelectedThumbnail();
		});

	auto nextText = make_shared<Label>(
		Pos{ 995, 138 },
		0, 0,
		"次へ",
		BLACK
	);

	// 遊ぶ
	auto playBtn = make_shared<Button>(
		Pos{ 440, 470 },
		160,
		55
	);

	playBtn->SetOnClick([this]() {
		changeFlg = 1;
		});

	auto playText = make_shared<Label>(
		Pos{ 490, 488 },
		0, 0,
		"遊ぶ",
		BLACK
	);

	// 作る
	auto createBtn = make_shared<Button>(
		Pos{ 680, 470 },
		160,
		55
	);

	createBtn->SetOnClick([this]() {
		changeFlg = 2;
		});

	auto createText = make_shared<Label>(
		Pos{ 730, 488 },
		0, 0,
		"作る",
		BLACK
	);

	// タイトルへ戻る
	auto backBtn = make_shared<Button>(
		Pos{ 560, 565 },
		160,
		55
	);

	backBtn->SetOnClick([this]() {
		changeFlg = 3;
		});

	auto backText = make_shared<Label>(
		Pos{ 610, 583 },
		0, 0,
		"戻る",
		BLACK
	);

	// 上下・左右移動用
	prevBtn->SetGroupId(1);
	nextBtn->SetGroupId(1);

	playBtn->SetGroupId(2);
	createBtn->SetGroupId(2);

	backBtn->SetGroupId(3);

	uiManager.Add(prevBtn);
	uiManager.Add(prevText);

	uiManager.Add(nextBtn);
	uiManager.Add(nextText);

	uiManager.Add(playBtn);
	uiManager.Add(playText);

	uiManager.Add(createBtn);
	uiManager.Add(createText);

	uiManager.Add(backBtn);
	uiManager.Add(backText);
}

void Select::Init() {
	SetMouseDispFlag(TRUE);

	changeFlg = 0;
	first = false;

	titleMapFloorWarning = false;
	showStartGoalWarning = false;

	RefreshTitleMapNames();

	selectedMapIndex =
		gameCurrentMapName[0] != '\0'
		? FindTitleMapNameIndex(gameCurrentMapName)
		: 0;

	LoadSelectedThumbnail();

	UISet();
}

void Select::Draw()
{
	ClearDrawScreen();

	// 背景
	DrawBox(
		0, 0,
		WIDTH, HEIGHT,
		GetColor(30, 40, 60),
		TRUE
	);

	const int infoLeft = 360;
	const int infoTop = 70;
	const int infoRight = 920;
	const int infoBottom = 410;

	// マップ情報の背景
	DrawBox(
		infoLeft,
		infoTop,
		infoRight,
		infoBottom,
		GetColor(20, 30, 45),
		TRUE
	);

	DrawBox(
		infoLeft,
		infoTop,
		infoRight,
		infoBottom,
		GetColor(220, 235, 255),
		FALSE
	);

	// サムネイルを表示する範囲
	const int thumbnailLeft = 390;
	const int thumbnailTop = 90;
	const int thumbnailRight = 890;
	const int thumbnailBottom = 300;

	if (thumbnail_img != -1)
	{
		// 画像を指定した範囲に拡大・縮小して表示
		DrawExtendGraph(
			thumbnailLeft,
			thumbnailTop,
			thumbnailRight,
			thumbnailBottom,
			thumbnail_img,
			TRUE
		);
	}
	else
	{
		// サムネイルがない場合
		DrawBox(
			thumbnailLeft,
			thumbnailTop,
			thumbnailRight,
			thumbnailBottom,
			GetColor(45, 55, 70),
			TRUE
		);

		DrawString(
			565,
			185,
			"NO IMAGE",
			GetColor(180, 190, 200)
		);
	}

	const char* selectedMapName =
		GetSelectedTitleMapName(selectedMapIndex);

	// マップ名
	DrawFormatString(
		390,
		320,
		WHITE,
		"マップ名：%s",
		selectedMapName
	);

	MapInfo selectedMapInfo;

	if (LoadMapInfoByName(selectedMapName, selectedMapInfo))
	{
		if (selectedMapInfo.description[0] != '\0')
		{
			DrawFormatString(
				390,
				360,
				GetColor(220, 235, 255),
				"説明：%s",
				selectedMapInfo.description
			);
		}
		else
		{
			DrawString(
				390,
				360,
				"説明：なし",
				GetColor(180, 190, 200)
			);
		}
	}

	uiManager.Draw();

	// 警告画面
	if (titleMapFloorWarning)
	{
		DrawBox(
			300, 250,
			980, 420,
			GetColor(10, 10, 10),
			TRUE
		);

		DrawBox(
			300, 250,
			980, 420,
			WHITE,
			FALSE
		);

		DrawString(
			330, 282,
			"スタートとゴールは床の上に置いてください。",
			GetColor(255, 255, 0)
		);

		DrawString(
			330, 320,
			"このマップはまだ遊べません。",
			GetColor(220, 240, 255)
		);

		DrawString(
			330, 360,
			"決定キーを押して閉じる",
			GetColor(220, 240, 255)
		);
	}

	if (showStartGoalWarning)
	{
		DrawBox(
			300, 250,
			980, 420,
			GetColor(10, 10, 10),
			TRUE
		);

		DrawBox(
			300, 250,
			980, 420,
			WHITE,
			FALSE
		);

		DrawString(
			330, 282,
			"スタートからゴールまで繋がっていません。",
			GetColor(255, 255, 0)
		);

		DrawString(
			330, 320,
			"このマップはまだ遊べません。",
			GetColor(220, 240, 255)
		);

		DrawString(
			330, 360,
			"決定キーを押して閉じる",
			GetColor(220, 240, 255)
		);
	}
}

void Select::Update(SceneManager& manager) {
	if (titleMapFloorWarning)
	{
		if (Input::IsKeyTrigger(KEY_INPUT_RETURN) ||
			Input::IsKeyTrigger(KEY_INPUT_SPACE) ||
			Input::IsKeyTrigger(KEY_INPUT_C) ||
			Input::IsKeyTrigger(KEY_INPUT_F9) ||
			Input::IsKeyTrigger(KEY_INPUT_Y) ||
			Input::IsKeyTrigger(KEY_INPUT_N))
		{
			titleMapFloorWarning = false;
		}

		return;
	}

	if (showStartGoalWarning)
	{
		if (Input::IsKeyTrigger(KEY_INPUT_RETURN) ||
			Input::IsKeyTrigger(KEY_INPUT_SPACE) ||
			Input::IsKeyTrigger(KEY_INPUT_C) ||
			Input::IsKeyTrigger(KEY_INPUT_F9) ||
			Input::IsKeyTrigger(KEY_INPUT_Y) ||
			Input::IsKeyTrigger(KEY_INPUT_N))
		{
			showStartGoalWarning = false;
		}

		return;
	}

	if (!first)
	{
		uiManager.SetSoundManager(
			&manager.GetSoundManager()
		);

		first = true;
	}

	switch (changeFlg)
	{
		// 選択したマップで遊ぶ
	case 1:
		ApplySelectedMap();
		LoadMapByName(gameCurrentMapName);

		if (!IsEditorMapStartGoalOnFloor())
		{
			titleMapFloorWarning = true;
			changeFlg = 0;
			break;
		}

		manager.GetSoundManager().Play("confirm");

		manager.ChangeScene(
			make_unique<Game>(),
			make_unique<Fade>()
		);

		return;

		// 選択したマップを編集する
	case 2:
		ApplySelectedMap();
		manager.GetSoundManager().Play("confirm");

		manager.ChangeScene(
			make_unique<Create>(),
			make_unique<Slide>()
		);

		return;

		// タイトルへ戻る
	case 3:
		manager.GetSoundManager().Play("cancel");

		manager.ChangeScene(
			make_unique<Title>(),
			make_unique<Slide>()
		);

		return;
	}

	uiManager.Update();
}

Game::Game(GameLaunchMode launchMode) : launchMode(launchMode) {
	Init();
}

void Game::UISet() {
	BuildPauseUI();
}

void Game::Init() {
	isPause = false;
	goTitle = false;

	confirmSaveOnTitle = false;

	first = false;

	if (gameCurrentMapName[0] != '\0')
	{
		LoadMapByName(gameCurrentMapName);
	}

	Input::CenterMouse();

	restart = false;
	goOption = false;
	showingOption = false;

	timer.Reset();
	UISet();
	InitGameMap();
	// 2026-06-02: マップ読込後のEVENT配置からゲーム中ギミックを生成するため追加。
	InitGameObjects();
	// 2026-06-12: [ENEMY]???????G???AEVENT?M?~?b?N???????????????????B
	//InitGameEnemies();
	p.Init();
	//e.Init();

}

void Game::Draw() {
	p.Draw();
	// 2026-07-23: 敵デバッグ文字は出さず、現在階層の敵本体だけ描画する。
	//e.DrawLayer(GetGameDrawLayer());
	//DrawString(WIDTH / 2, HEIGHT-550, "ゲーム画面", GetColor(255, 255, 255));
	DrawGameMap();
	// 2026-05-18: 迷路探索中に階層と通った道が見えるよう、3D描画の後にミニマップを重ねる。
	// 2026-06-02: 取るもの/鍵/敵弾などを3D上に表示し、状態HUDも出すため追加。
	DrawGameObjects();
	// 2026-06-12: ?G?{???G?e??EVENT?`?悩?番????`?????????B
	//DrawGameEnemies();
	DrawMiniMap(p.getVECTOR());
	DrawGameObjectHUD();
	DrawFormatString(20, 92, WHITE, "LIFE %d", p.GetLife());
	DrawFormatString(20, 122, WHITE, "layer %d", p.GetLayer());

	elapsed = (timer.Elapsed() / 1000) % 60;
	int minutes = timer.Elapsed() / 60000;

	DrawFormatStringToHandle(WIDTH / 2, 100, WHITE, FontManager::Get(32), "%02d:%02d", minutes, elapsed);

	// 2026-07-15: 階層をまたぐ時は画面全体を暗転させ、1層ずつ切り替わったように見せる。
	DrawGameLayerFadeOverlay();

	if (isPause) {
		if (showingOption) {
			SetDrawBlendMode(DX_BLENDMODE_ALPHA, 200);
			DrawBox(0, 0, WIDTH, HEIGHT, BLACK, TRUE);
			SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

			optionUI.Draw();
		}
		else {
			DrawOption();
			uiManager.Draw();
		}
	}
}

void Game::Update(SceneManager& manager) {
	if (!first) {
		manager.GetSoundManager().Play("bgm2");
		first = true;
	}

	if (Input::IsActionTrigger(Action::Pause)) {
		if (showingOption) {
			showingOption = false;
		}
		else {
			isPause = !isPause;
		}

		Input::CenterMouse();
	}

	if (!isPause)
	{
		SetMouseDispFlag(FALSE);
		Input::UpdateMouseDeltaFromCenter();

		p.Update();
		e.Update();
		c.Update();
		UpdateGameMap();

		timer.Resume();

		// 2026-06-02: 取るもの/避けるもの/鍵扉/投げ敵をプレイヤー位置に合わせて更新するため追加。
		bool playerDamaged = false;
		int playerLayer = GetMapLayerFromWorldY(p.getVECTOR().y + BLOCK_SIZE * 0.5f);
		UpdateGameObjects(p.getVECTOR(), playerLayer, playerDamaged);
		// 2026-06-12: ?GAI??G?e????????????X?V?????????B
	//	UpdateGameEnemies(p.getVECTOR(), playerLayer, playerDamaged);
		if (playerDamaged)
		{
			timer.Pause();
			p.Damage();
			manager.Trans(make_unique<DamageFade>(p.GetLife()));
		}
		
		// 2026-06-02: ライフが尽きた時にゲームを終わらせるため追加。
		if (!p.IsAlive())
		{
			timer.Pause();
			manager.ChangeScene(make_unique<Result>(timer.Elapsed(), launchMode, GameResult::Lose), make_unique<Fade>());
			return;
		}

		// 2026-06-02: 鍵または収集条件を満たした時だけゴールできるよう追加。
		// 6/19 変更
		if (IsGoal())
		{
			timer.Pause();
			manager.ChangeScene(make_unique<Result>(timer.Elapsed(), launchMode, GameResult::Win), make_unique<Goal>());
			return;
		}
	}
	else {
		SetMouseDispFlag(TRUE);
		uiManager.SetSoundManager(&manager.GetSoundManager());
		timer.Pause();
		if (showingOption) {
			optionUI.SetSoundManager(&manager.GetSoundManager());
			optionUI.Update();

			if (optionUI.IsBackRequested()) {
				optionUI.ResetBackRequest();
				showingOption = false;
				BuildPauseUI();
			}
		}
		else {
			uiManager.Update();

			if (restart) {
				manager.GetSoundManager().Play("confirm");
				manager.ChangeScene(
					make_unique<Game>(),
					make_unique<Slide>()
				);
				return;
			}

			if (goTitle) {
				manager.GetSoundManager().Play("confirm");
				if (launchMode == GameLaunchMode::EditorTestPlay)
					manager.ChangeScene(make_unique<Create>(), make_unique<Slide>());
				else
					manager.ChangeScene(make_unique<Title>(), make_unique<Slide>());
				return;
			}

			if (goOption) {
				manager.GetSoundManager().Play("confirm");
				goOption = false;
				showingOption = true;
				optionUI.Init();
				return;
			}
		}
		return;
	}
}

void Game::DrawOption() {
	SetUseZBuffer3D(FALSE);
	SetWriteZBuffer3D(FALSE);

	// 背景を少し暗くする
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 100);
	DrawBox(0, 0, WIDTH, HEIGHT, BLACK, TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

	// 中央パネル
	int panelX = 440;
	int panelY = 140;
	int panelW = 400;
	int panelH = 460;

	DrawBox(panelX, panelY, panelX + panelW, panelY + panelH, GetColor(20, 20, 30), TRUE);
	DrawBox(panelX, panelY, panelX + panelW, panelY + panelH, WHITE, FALSE);

	DrawString(panelX + 145, panelY + 35, "PAUSE", WHITE);

	SetUseZBuffer3D(TRUE);
	SetWriteZBuffer3D(TRUE);
}

void Game::BuildPauseUI() {
	uiManager.Clear();
	uiManager.SetScrollEnabled(false);

	goTitle = false;
	restart = false;
	goOption = false;

	int panelX = 440;
	int panelY = 140;

	int btnX = panelX + 75;
	int btnY = panelY + 110;
	int btnW = 250;
	int btnH = 60;
	int gap = 80;

	auto backBtn = make_shared<Button>(Pos{ btnX, btnY }, btnW, btnH);
	backBtn->SetOnClick([this]() {
		isPause = false;
		});

	auto backText = make_shared<Label>(Pos{ btnX + 95, btnY + 20 }, 0, 0, "再開", BLACK, 20);

	auto optionBtn = make_shared<Button>(Pos{ btnX, btnY + gap }, btnW, btnH);
	optionBtn->SetOnClick([this]() {
		goOption = true;
		});

	auto optionText = make_shared<Label>(Pos{ btnX + 95, btnY + gap + 20 }, 0, 0, "設定", BLACK, 20);

	auto restartBtn = make_shared<Button>(Pos{ btnX, btnY + gap * 2 }, btnW, btnH);
	restartBtn->SetOnClick([this]() {
		restart = true;
		});

	auto restartText = make_shared<Label>(Pos{ btnX + 75, btnY + gap * 2 + 20 }, 0, 0, "リスタート", BLACK, 20);

	auto titleBtn = make_shared<Button>(Pos{ btnX, btnY + gap * 3 }, btnW, btnH);
	titleBtn->SetOnClick([this]() {
		goTitle = true;
		});

	const char* exitLabel = launchMode == GameLaunchMode::EditorTestPlay ? "編集に戻る" : "タイトルへ";
	auto titleText = make_shared<Label>(Pos{ 580, 510 }, 0, 0, exitLabel, BLACK, 20);


	backBtn->SetGroupId(0);
	optionBtn->SetGroupId(1);
	restartBtn->SetGroupId(2);
	titleBtn->SetGroupId(3);

	uiManager.Add(backBtn);
	uiManager.Add(backText);
	uiManager.Add(optionBtn);
	uiManager.Add(optionText);
	uiManager.Add(restartBtn);
	uiManager.Add(restartText);
	uiManager.Add(titleBtn);
	uiManager.Add(titleText);
}
// 6/19追加
// ゴールの判定長いからこれにまとめる
bool Game::IsGoal() {
	// 2026-07-15: ジャンプ高さで上階扱いになると、1階から2階ゴールに触れた判定になるため実際の所属レイヤーを見る。
	int playerLayer = p.GetLayer();
	return IsGoalWorldPosition(playerLayer, p.getVECTOR().x, p.getVECTOR().z) && IsGameObjectGoalUnlocked();
}

Result::Result(int time, GameLaunchMode launchMode, GameResult result) : clearTime(time), launchMode(launchMode), result(result) {
	Init();
	SetMouseDispFlag(TRUE);
}

void Result::UISet() {
	auto titleBtn = make_shared<Button>(Pos{ 400, 600 }, 150, 50);
	titleBtn->SetOnClick([this]() {
		changeflg = 1;
		});

	const char* exitLabel = launchMode == GameLaunchMode::EditorTestPlay ? "編集に戻る" : "タイトルへ";
	auto title_text = make_shared<Label>(Pos{ 425, 620 }, 0, 0, exitLabel, BLACK, 16);

	auto retryBtn = make_shared<Button>(Pos{ 700, 600 }, 150, 50);
	retryBtn->SetOnClick([this]() {
		changeflg = 2;
		});

	auto retry_text = make_shared<Label>(Pos{ 750, 620 }, 0, 0, "リトライ", BLACK);

	titleBtn->SetGroupId(1);
	retryBtn->SetGroupId(1);

	uiManager.Add(titleBtn);
	uiManager.Add(title_text);
	uiManager.Add(retryBtn);
	uiManager.Add(retry_text);
}

void Result::Init() {
	uiManager.Clear();
	uiManager.SetScrollEnabled(false);

	switch (result) {
	case GameResult::Win:
		goalImg = LoadGraph(resultGOAL_image.path);
		break;
	case GameResult::Lose:
		goalImg = LoadGraph(resultLose_image.path);
		break;
	}

	totalTime = 0;
	elapsed = 0;
	minutes = 0;

	changeflg = 0;

	goalScale = 0.0f;
	goalScaleBack = false;

	totalTime = 0;
	countTimer = 0;

	first = false;
	UISet();

}

void Result::Draw() {
	switch (result) {
	case GameResult::Win:
		DrawRotaGraph(WIDTH / 2, 50 + resultGOAL_image.h / 2, goalScale, 0.0, goalImg, true);
		DrawFormatString(300, 400, WHITE, "CLEAR TIME : %02d:%02d", minutes, elapsed);
		break;
	case GameResult::Lose:
		DrawRotaGraph(WIDTH / 2, HEIGHT / 2 - 50, 0.5f, 0.0, goalImg, true);
		break;
	}

	uiManager.Draw();
}

void Result::Update(SceneManager& manager) {
	if (!first) {
		manager.GetSoundManager().Play("bgm3");
		first = true;
	}

	uiManager.Update();

	switch (result) {
	case GameResult::Win:
		ScaleUpAnimation();
		CountUp();
		break;
	case GameResult::Lose:

		break;
	}



	switch (changeflg) {
	case 1:
		manager.GetSoundManager().Play("confirm");
		if (launchMode == GameLaunchMode::EditorTestPlay)
			manager.ChangeScene(make_unique<Create>(), make_unique<Slide>());
		else
			manager.ChangeScene(make_unique<Title>(), make_unique<Slide>());
		break;
	case 2:
		manager.GetSoundManager().Play("confirm");
		manager.ChangeScene(
			make_unique<Game>(launchMode),
			make_unique<Slide>()
		);
		break;
	}
}

void Result::ScaleUpAnimation() {
	if (!goalScaleBack)
	{
		goalScale += 0.05f;
		if (goalScale >= 1.15f) {
			goalScale = 1.15f;
			goalScaleBack = true;
		}
	}
	else
	{
		goalScale += (1.0f - goalScale) * 0.15f;
		if (fabsf(goalScale - 1.0f) < 0.01f) {
			goalScale = 1.0f;
		}
	}
}

void Result::CountUp() {
	if (!ScaleUpEnd())return;
	if (countTimer < countDuration)
	{
		countTimer++;

		float t = (float)countTimer / countDuration;

		t = 1.0f - (1.0f - t) * (1.0f - t);

		totalTime = (int)(clearTime * t);
	}
	else
	{
		totalTime = clearTime;
	}

	elapsed = (totalTime / 1000) % 60;
	minutes = totalTime / 60000;
}



Create::Create() {
	Init();
}

void Create::UISet() {
	// 2026-05-19: エディターの戻るボタンは固定表示にしたいので、ホイールスクロールで動かないようにする。
	uiManager.SetScrollEnabled(false);

	auto titleBtn = make_shared<Button>(Pos{ 650, 10 }, 130, 36);
	titleBtn->SetOnClick([this]() {
		goTitle = true;
		});
	auto playCurrentMapBtn = make_shared<Button>(Pos{ 470, 10 }, 160, 36);
	playCurrentMapBtn->SetOnClick([this]() {
		playCurrentMap = true;
		});

	auto playCurrentMapText = make_shared<Label>(Pos{ 500, 20 }, 0, 0, "テストプレイ", BLACK);

	auto titleText = make_shared<Label>(Pos{ 682, 20 }, 0, 0, "戻る", BLACK);

	uiManager.Add(titleBtn);
	uiManager.Add(titleText);
	uiManager.Add(playCurrentMapBtn);
	uiManager.Add(playCurrentMapText);
}

void Create::Init() {
	first = false;

	SetMouseDispFlag(TRUE);
	InitEditor();
	uiManager.Clear();
	// 2026-07-15: 復旧時にUISet呼び出しが抜け、テストプレイ/タイトルボタンが表示されなくなっていたため戻す。
	UISet();
	playCurrentMap = false;
	playMapFloorWarning = false;
	confirmSaveOnTitle = false;
	goTitle = false;
}

void Create::Update(SceneManager& manager) {
	if (!first) {
		manager.GetSoundManager().Play("bgm9");
		first = true;
	}

	if (playMapFloorWarning) {
		// 2026-07-16: テストプレイ不可の警告中はエディター操作を止め、キー入力で閉じる。
		if (Input::IsKeyTrigger(KEY_INPUT_RETURN) || Input::IsKeyTrigger(KEY_INPUT_SPACE) ||
			Input::IsKeyTrigger(KEY_INPUT_C) || Input::IsKeyTrigger(KEY_INPUT_F9) ||
			Input::IsKeyTrigger(KEY_INPUT_Y) || Input::IsKeyTrigger(KEY_INPUT_N)) {
			playMapFloorWarning = false;
		}
		return;
	}
	if (confirmSaveOnTitle) {
		// 2026-06-12: 未保存のままタイトルへ戻る時、編集内容を破棄してよいか確認するため追加。
		if (Input::IsKeyTrigger(KEY_INPUT_Y)) {
			ClearEditorMapDirty();
			confirmSaveOnTitle = false;
			manager.ChangeScene(make_unique<Select>(), make_unique<Slide>());
			return;
		}
		if (Input::IsKeyTrigger(KEY_INPUT_N) || Input::IsKeyTrigger(KEY_INPUT_C) || Input::IsKeyTrigger(KEY_INPUT_F9)) {
			confirmSaveOnTitle = false;
			goTitle = false;
			return;
		}

		return;
	}
	uiManager.Update();

	if (goTitle) {
		// 2026-06-22: 名前とか入力中は起動しない
		if (IsEditorSaveInputActive()) {
			goTitle = false;
			return;
		}
		// 2026-05-27: 変更がある時は、タイトルへ戻る前に保存確認を出す。
		// 2026-06-12: 保存するかではなく、保存せず戻るかの確認に変更。
		if (IsEditorMapDirty()) {
			confirmSaveOnTitle = true;
			goTitle = false;
			return;
		}
		manager.ChangeScene(make_unique<Select>(), make_unique<Slide>());
		return;
	}
	if (playCurrentMap) {
		// 2026-06-15: エディターとゲーム処理は分けたまま、現在のマップを保存してGameシーンへ渡すため追加。
		playCurrentMap = false;
		// 2026-06-22: 名前とか入力中は起動しない
		if (IsEditorSaveInputActive()) {
			return;
		}
		if (!IsEditorMapStartGoalOnFloor()) {
			playMapFloorWarning = true;
			return;
		}
		if (IsEditorMapDirty()) {
			SaveMap();
		}
		if (gameCurrentMapName[0] == '\0') {
			gameCurrentMapIndex = currentMapIndex;
		}
		manager.ChangeScene(make_unique<Game>(GameLaunchMode::EditorTestPlay), make_unique<Slide>());
		return;
	}



	UpdateEditor();
}

void Create::Draw() {
	DrawEditor();
	uiManager.Draw();


	if (playMapFloorWarning) {
		// 2026-06-15: START/GOALが床の上にない時、プレイできない理由を画面に出すため追加。
		DrawBox(300, 250, 980, 420, GetColor(10, 10, 10), TRUE);
		DrawBox(300, 250, 980, 420, GetColor(255, 255, 255), FALSE);
		DrawString(330, 282, "スタートとゴールは床の上に置いてください。", GetColor(255, 255, 0));
		DrawString(330, 320, "床がない場所からは、このマップで遊べません。", GetColor(220, 240, 255));
		DrawString(330, 360, "Enter/Space/C/F9: 閉じる", GetColor(220, 240, 255));
	}
	else if (confirmSaveOnTitle) {
		// 2026-06-15: タイトルへ戻る確認中も、警告が見えるよう描画を戻す。
		DrawBox(300, 260, 980, 400, GetColor(10, 10, 10), TRUE);
		DrawBox(300, 260, 980, 400, GetColor(255, 255, 255), FALSE);
		DrawString(330, 292, "保存されてません。保存せず戻りますか？", GetColor(255, 255, 0));
		DrawString(330, 330, "Y: 保存せず戻る   N/C/F9: キャンセル", GetColor(220, 240, 255));
	}
}

OptionMenu::OptionMenu() {
	Init();
}

void OptionMenu::UISet() {
	optionUI.Init();
}

void OptionMenu::Init() {
	first = false;
	UISet();
	SetMouseDispFlag(TRUE);
}

void OptionMenu::Update(SceneManager& manager) {
	if (!first) {
		manager.GetSoundManager().Play("bgm6");
		first = true;
	}
	optionUI.SetSoundManager(&manager.GetSoundManager());
	uiManager.SetSoundManager(&manager.GetSoundManager());
	optionUI.Update();

	if (optionUI.IsBackRequested()) {
		manager.ChangeScene(
			make_unique<Title>(),
			make_unique<Slide>()
		);
	}
}

void OptionMenu::Draw() {
	optionUI.Draw();
}



