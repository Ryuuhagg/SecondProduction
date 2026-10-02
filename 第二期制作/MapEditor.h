#pragma once
#include "MapData.h"

#pragma region ===== 初期化 =====

void InitEditor();

#pragma endregion


#pragma region ===== 更新 =====

void UpdateEditor();

#pragma endregion


#pragma region ===== 描画 =====

void DrawEditor();

#pragma endregion


#pragma region ===== CSV保存 / 読込 =====

void SaveMap();
void SaveMapAsCurrentName();
void LoadMap(int mapIndex);
void LoadMapByName(const char* mapName);
bool FindEditorEventPosition(int eventId, int& layer, int& z, int& x);
bool SyncEditorStartGoalFromEvents();
bool IsEditorMapDirty();
bool IsEditorSaveInputActive();
void MarkEditorMapDirty();
void ClearEditorMapDirty();
bool SaveEditorMapThumbnail(const char* thumbnailPath);

struct MapInfo
{
    char name[64];
    char csv[64];
    char thumbnail[260];
    char description[256];
};

bool LoadMapInfoByName(const char* mapName, MapInfo& info);
//void RequestEditorLoadMap(const char* mapName);
#pragma endregion


#pragma region ===== Undo =====

void PushUndo();
void UndoMap();

#pragma endregion


#pragma region ===== 壁処理 =====

void AutoConnectWalls();
void UpdateWallTile(int y, int z, int x);

#pragma endregion


#pragma region ===== UI =====

void DrawEditorUI();
void UpdateEditorUI(int mx, int my, int lClick);
bool UpdateEditorUIWheel(int mx, int my, int wheel);
bool IsEditorWorkspacePoint(int mx, int my);
void SaveEditorFromUI();
void NewEditorMapFromUI();
void RefreshEditorKitList();
int GetEditorKitCount();
const char* GetEditorKitName(int index);
int GetSelectedEditorKitIndex();
int GetEditorKitThumbnailHandle(int index);
bool IsEditorKitFromCsv(int index);
bool RebuildEditorBaseKitCsv();
bool RefreshSelectedEditorKitThumbnail();
void SelectEditorKit(int index);
void StartEditorKitRegistrationFromSelection();
int GetEditorEventIdFromModelForUI(int modelId);
// 2026-07-21: 右パネルの範囲選択ボタンを、範囲作成後はコピー開始ボタンとして使うため追加。
bool HasEditorRangeSelection();
void ClearEditorRangeSelection();
bool StartEditorCopyFromSelectionButton();
// 2026-07-21: 右パネルの色変更ボタンから、選択範囲や複数選択へ現在色をまとめて適用する。
bool ApplyEditorColorToSelection();

#pragma endregion


#pragma region ===== 共通処理 =====

void ClearCurrentLayer();
void ResetAllMap();
extern int CollisionCornerScaleMap[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];
extern int CollisionCornerThicknessMap[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];
extern char gameCurrentMapName[64];
extern char gameCurrentMapDescription[256];

#pragma endregion

