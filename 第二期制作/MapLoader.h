#pragma once
#include "DxLib.h"
#include "MapData.h"
#include"Constant.h"
void InitGameMap();
void LoadGameMap();
void UpdateGameMap();
void DrawGameMap();
// 2026-05-18: ゲーム中に現在階層と通った道を確認できるミニマップを描くため追加。
void DrawMiniMap(VECTOR playerPos);

bool HasWallEdge(int y, int z, int x, int edge);
bool CanMoveCellToCell(int y, int fromX, int fromZ, int toX, int toZ);
bool CanMoveWorldPosition(int y, float worldX, float worldZ, float radius);
bool CanMoveWorldPosition(int y, float worldX, float worldZ, float radius, float currentY);
bool CanCameraMoveWorldPosition(int y, float worldX, float worldZ, float radius, float currentY);
bool IsGoalWorldPosition(int y, float worldX, float worldZ);
VECTOR ResolvePlayerMapCollision(VECTOR currentPos, VECTOR nextPos, float radius, int currentLayer);
VECTOR ResolvePlayerMapCollision(VECTOR currentPos, VECTOR nextPos, float radius, float currentY, int currentLayer);
VECTOR ResolvePlayerMapCollisionForPlayer(VECTOR currentPos, VECTOR nextPos, float radius, float jumpY, int currentLayer);
void UpdatePlayerMapVertical(VECTOR& pos, float& jumpY, float& vy, bool& isGround, int& layer, bool jumpTrigger, bool stepDownTrigger, float gravity, float groundRadius);

//佐藤龍波が追加
VECTOR ResolveCameraCollision(VECTOR target, VECTOR ideal, float radius, int layer);

float GetMapGroundY(float worldX, float worldZ);
float GetMapGroundY(float worldX, float worldZ, float currentY);
float GetMapGroundY(float worldX, float worldZ, float currentY, float groundRadius);
bool IsStairsAtWorld(float worldX, float worldZ);
// 2026-06-29: 階段判定で別階層の同座標を拾わないよう、指定階層だけ確認する。
bool IsStairsAtWorldLayer(int layer, float worldX, float worldZ);
// 2026-05-20: 階段上で、1マス先として先に見せるべき階層を取得する。
bool TryGetStairsLoadLayer(int currentLayer, float worldX, float worldZ, int& loadLayer);
bool TryMoveLayerByStairs(int& layer, float worldX, float worldZ);
bool IsGoalCell(int y, int z, int x);

extern int GetMapLayerFromWorldY(float worldY);
int GetMapGroundLayerFromY(float groundY);
bool IsMapHalfStepGroundY(float groundY);
// 2026-05-20: 階段で層をまたいだ時に、ゲーム側のロード対象層を切り替える。
void SetGameLoadedLayer(int layer);
int GetGameLoadedLayer();
int GetGameDrawLayer();
void DrawGameLayerFadeOverlay();

extern int gameCurrentMapIndex;
// 2026-05-27: タイトル画面で選んだCSV名をゲーム側ロードに渡すため追加。
extern char gameCurrentMapName[64];
extern int gameLoadedLayer;
// 2026-06-02: GameObjectsがCSVから読んだEVENT配置を元にギミックを生成するため追加。
extern int GameEventMap[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];
// 2026-07-21: CSVから読んだEVENT回転をGameObjects側の描画へ渡すため公開する。
extern int GameEventRot[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];
// 2026-06-11: GameObjectsではなくGameEnemiesが[ENEMY]配置を生成できるよう追加。
extern int GameEnemyMap[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];
// 2026-07-06: GameObjects側でもmodel_config.csvのEVENT表示モデルを参照できるよう公開する。
extern int gameModelHandles[MODEL_MAX];
int GetGameEventModelId(int eventId);

extern int gameStartX;
extern int gameStartY;
extern int gameStartZ;

extern int gameGoalX;
extern int gameGoalY;
extern int gameGoalZ;

VECTOR GetStartPosition();

// 7/17 階層移動先を取得する関数
bool GetLayerMoveDestination(
    int currentLayer,
    int currentX,
    int currentZ,
    int direction,
    MapNode& destination);


