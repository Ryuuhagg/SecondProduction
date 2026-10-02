#include "MapEditor.h"
#include "Input.h"
#include "CsvUtil.h"
// #include "GameEnemies.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <windows.h>
#include <vector>
#include <math.h>
#include <direct.h>

using namespace std;

#pragma region ===== マップ配列 =====

int FloorMap[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];
int FloorRot[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];
// 2026-07-21: 色変更ボタンで選んだ色を、配置済み床ごとに保存する。
int FloorColorMap[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];

int WallMapA[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];
int WallMapB[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];
int WallRotA[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];
int WallRotB[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];
// 2026-07-21: 壁はA/Bの2枠があるため、色も別々に保持する。
int WallColorMapA[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];
int WallColorMapB[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];

int CornerMap[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];
int CornerRot[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];
// 2026-07-21: 角モデルの色変更を配置単位で保持する。
int CornerColorMap[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];

int DecoMap[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];
int DecoRot[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];
// 2026-07-21: 装飾モデルの色変更を配置単位で保持する。
int DecoColorMap[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];
// 2026-07-16: 0.5床の上層接続方向をエディターで指定できるよう追加。
int ClimbLinkMap[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];

// 2026-06-11: 敵配置をEventMapへ混ぜず、[ENEMY]として保存できるよう追加。
int EnemyMap[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];
int EnemyPatrolCountMap[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];
int EnemyPatrolXMap[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X][ENEMY_PATROL_POINT_MAX];
int EnemyPatrolZMap[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X][ENEMY_PATROL_POINT_MAX];
int EventMap[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];
// 2026-07-21: 鍵扉などのEVENTモデルをRキーの向きで置けるよう、EVENT専用の回転値を追加。
int EventRot[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];
// 2026-05-11: エディターでセル単位の手動当たり判定を持たせ、CSV経由でLoaderへ渡すため追加。
int CollisionMap[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];
// 2026-05-20: UnityのBoxColliderのように手動当たり判定を直接触って調整できるよう、中心オフセットとサイズを持たせる。
int CollisionBoxOffsetXMap[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];
int CollisionBoxOffsetZMap[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];
int CollisionBoxSizeXMap[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];
int CollisionBoxSizeZMap[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];
// 2026-05-11: Loaderの壁ライン判定をエディターで直接見て編集できるよう、辺単位の手動当たり判定を追加。
int CollisionEdgeMap[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];
// 2026-05-11: 辺当たり判定を伸ばしたり縮めたりする編集値を持つため追加。100が通常の長さ。
int CollisionEdgeScaleMap[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X][4];
// 2026-05-11: 辺当たり判定の厚さを編集できるよう、100を通常の厚さとして追加。
int CollisionEdgeThicknessMap[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X][4];
// 2026-05-13: コーナー当たり判定の長さ/厚みをエディターで調整・保存するため追加。
int CollisionCornerScaleMap[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];
int CollisionCornerThicknessMap[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];
#pragma endregion


#pragma region ===== モデル別の既定当たり判定 =====

enum EditorCollisionType
{
    EDITOR_COLL_NONE,
    EDITOR_COLL_CIRCLE,
    EDITOR_COLL_BOX,
    EDITOR_COLL_ARC,
    EDITOR_COLL_WALL,
    EDITOR_COLL_STAIRS
};

struct EditorCollisionInfo
{
    EditorCollisionType type;
    float radius;
    float width;
    float depth;
};

// 2026-05-11: オブジェクトを置いた時点で既定の当たり判定を持たせ、エディターで見えるようにするため追加。
EditorCollisionInfo editorCollisionTable[MODEL_MAX];
#pragma endregion


#pragma region ===== エディタ状態 =====

int currentLayer = 0;
int currentRot = 0;
int currentMapIndex = 1;
// 2026-05-26: 最初の保存時に入力した名前を保持し、次回以降は同じCSVへ上書き保存する。
char gameCurrentMapName[64] = "";
// 2026-07-17: マップ説明を保存UIとinfo.txtの間で共有し、タイトルなどから後で読み出せるよう保持する。
char gameCurrentMapDescription[256] = "";
//char mapNameList[256][64];

//int mapNameCount = 0;

//int selectedMapListIndex = 0;
// 2026-05-26: 左右キーで選ぶマップ名。番号ではなくCSV名を表示して読み込むため追加。
char selectedMapName[64] = "";
static const int MAP_NAME_LIST_MAX = 64;
static char mapNameList[MAP_NAME_LIST_MAX][64];
static int mapNameCount = 0;
static int selectedMapListIndex = 0;

int currentTab = FLOOR;
int selectedModel = 0;
// 2026-06-02: 通常配置とコライダー調整を別画面として切り替え、操作が混ざらないようにするため追加。
int editorScreenMode = 0;
// 2026-06-02: EVENTなど素材数が増えた時に、右側の素材一覧をマウスホイールで下まで見られるようにするため追加。
int editorPaletteScrollY = 0;

int hoverX = -1;
int hoverZ = -1;
// 2026-05-20: BoxCollider風編集で、セル中心ではなくマウスが当たったワールド座標を使って直接触れるようにする。
float hoverWorldX = 0.0f;
float hoverWorldZ = 0.0f;

bool showGrid = true;
// 2026-07-21: 右パネルの色変更ボタンで選んだ色番号。0は通常色。
int editorColorIndex = 0;
// 2026-07-21: 消しゴムボタンON中は、左クリック配置を削除操作として扱う。
bool eraserMode = false;
bool eraserAllMode = false;
int eraserTarget = ERASER_TARGET_FLOOR;
bool brushMode = false;
// 2026-07-08: 置いてある敵ごとに巡回ポイントを編集できるよう、ENEMYタブ用の編集状態を追加。
bool enemyPatrolEditMode = false;
int selectedPatrolEnemyLayer = -1;
int selectedPatrolEnemyX = -1;
int selectedPatrolEnemyZ = -1;
// 2026-07-07: 編集中に現在の層だけを見られるよう、他階層の表示を切り替える。
bool showCurrentLayerOnly = false;
// 2026-05-11: 当たり判定編集と確認表示を、配置作業中に切り替えられるよう追加。
bool showCollisionDebug = false;
bool collisionEditMode = false;
// 2026-05-11: 当たり判定編集時にセル編集/辺編集を切り替えるため追加。
bool collisionEdgeEditMode = false;
// 2026-05-13: 辺/コーナー当たりのホイール編集を長さモード/厚みモードで切り替えるため追加。
bool collisionDepthEditMode = false;

// 2026-05-20: セル当たり判定のBOXをドラッグ中かどうかを保持し、モデルを直接触る編集にする。
bool collisionBoxDragging = false;
int collisionBoxDragMode = 0;
int collisionBoxDragLayer = -1;
int collisionBoxDragX = -1;
int collisionBoxDragZ = -1;
// 2026-06-02: BOX移動時にクリック位置へ中心が吸い寄せられないよう、つかんだ位置との差分を保持するため追加。
float collisionBoxDragOffsetX = 0.0f;
float collisionBoxDragOffsetZ = 0.0f;
// 2026-05-20: 右側UIで選択中コライダーを調整できるよう、選択セルを保持する。
int selectedCollisionLayer = -1;
int selectedCollisionX = -1;
int selectedCollisionZ = -1;
// 2026-05-26: F5初回保存時にCSVファイル名をキーボード入力するための状態。
bool saveNameInputActive = false;
char saveNameInput[64] = "";
// 2026-07-17: 保存時にマップ説明も入力できるよう、名前とは別の入力欄を持つ。
char saveDescriptionInput[256] = "";
int saveInputField = 0;
int saveDescriptionKeyInput = -1;
// 2026-06-25: 選択範囲を名前付きキットとして登録するための入力状態。
bool kitNameInputActive = false;
char kitNameInput[64] = "";
// 2026-05-27: 変更後に別マップを開く/タイトルへ戻る前、保存確認を出すためのフラグ。
bool editorMapDirty = false;
bool loadMapConfirmActive = false;
int pendingLoadMapIndex = -1;
// 2026-06-12: 未保存のまま別CSVへ切り替える時、キャンセルしても選択中マップ名がずれないよう保留先を保持する。
static char PendingLoadMapName[64] = "";
static int PendingLoadMapListIndex = -1;
static const int EDITOR_KIT_LIST_MAX = 64;
static char editorKitNameList[EDITOR_KIT_LIST_MAX][64];
static int editorKitThumbnailHandle[EDITOR_KIT_LIST_MAX];
// 2026-06-26: ベースキットCSVと自作キットをUI上で別タブ表示できるよう、読み込み元を保持する。
static bool editorKitFromCsv[EDITOR_KIT_LIST_MAX];
static int editorKitCount = 0;
static int selectedEditorKitIndex = -1;
static bool pendingKitThumbnailSave = false;
static char pendingKitThumbnailName[64] = "";

int startX = -1, startY = -1, startZ = -1;
int goalX = -1, goalY = -1, goalZ = -1;

#pragma endregion


#pragma region ===== カメラ =====

float camRotY = 0.7f;
float camRotX = 0.4f;
float camDist = 6500.0f;
// 2026-07-21: 未選択時の左ドラッグで、マップ中央固定ではなく見ている方向基準で注視点を動かす。
float camTargetX = BLOCK_NUM_X * BLOCK_SIZE * 0.5f;
float camTargetZ = BLOCK_NUM_Z * BLOCK_SIZE * 0.5f;

#pragma endregion


#pragma region ===== リソース =====

int modelHandles[MODEL_MAX];
int paletteTex[MODEL_MAX];
// 2026-07-21: model_config.csvのoffsetX/offsetZ/offsetYでモデルごとの表示位置を調整する。
float modelOffsetX[MODEL_MAX];
float modelOffsetZ[MODEL_MAX];
float modelOffsetY[MODEL_MAX];

#pragma endregion


#pragma region ===== タブ設定 =====

int tabModelList[TAB_MAX][16] = {};
int tabModelCount[TAB_MAX] = { 0,0,0,0,0 };

#pragma endregion


#pragma region ===== 入力状態 =====

int oldMX = 0;
int oldMY = 0;
int oldClick = 0;
int oldMouse = 0;

#pragma endregion


#pragma region ===== ブラシ状態 =====

int lastBrushX = -1;
int lastBrushZ = -1;
int lastBrushLayer = -1;

#pragma endregion


#pragma region ===== 範囲選択状態 =====

bool selectMode = false;
bool selecting = false;



int selectStartX = -1;
int selectStartZ = -1;
int selectEndX = -1;
int selectEndZ = -1;
int selectLayer = 0;

// 2026-06-25: 離れたマスを複数選んで、選択中のマスだけまとめて回転できるよう追加。
bool multiSelectMap[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X] = {};
int multiSelectCount = 0;

#pragma endregion


#pragma region ===== コピー =====

const int COPY_MAX_X = BLOCK_NUM_X;
const int COPY_MAX_Z = BLOCK_NUM_Z;

int copySizeX = 0;
int copySizeZ = 0;
bool hasCopyData= false;

// 2026-06-24: コピーした範囲を確定またはキャンセルするまでマウスに追従させるため追加。
// 貼り付け前の確認と、コピー範囲全体の回転を可能にする。
bool pasteMode = false;
int copyRotation = 0;

int CopyFloorMap[COPY_MAX_Z][COPY_MAX_X];
int CopyFloorRot[COPY_MAX_Z][COPY_MAX_X];

int CopyWallMapA[COPY_MAX_Z][COPY_MAX_X];
int CopyWallMapB[COPY_MAX_Z][COPY_MAX_X];
int CopyWallRotA[COPY_MAX_Z][COPY_MAX_X];
int CopyWallRotB[COPY_MAX_Z][COPY_MAX_X];

int CopyCornerMap[COPY_MAX_Z][COPY_MAX_X];
int CopyCornerRot[COPY_MAX_Z][COPY_MAX_X];

int CopyDecoMap[COPY_MAX_Z][COPY_MAX_X];
int CopyDecoRot[COPY_MAX_Z][COPY_MAX_X];

// 2026-06-11: 範囲コピー/貼り付けでも敵配置をEVENTとは別に保持するため追加。
int CopyEnemyMap[COPY_MAX_Z][COPY_MAX_X];
int CopyEventMap[COPY_MAX_Z][COPY_MAX_X];
// 2026-07-21: 扉入りキットや範囲コピーでもEVENTの向きが戻らないよう、コピー用回転値を追加。
int CopyEventRot[COPY_MAX_Z][COPY_MAX_X];
// 2026-05-11: 範囲コピー/貼り付けでも手動当たり判定を一緒に扱うため追加。
int CopyCollisionMap[COPY_MAX_Z][COPY_MAX_X];
// 2026-05-20: 範囲コピー/貼り付けでもBoxCollider風の調整値を一緒に扱うため追加。
int CopyCollisionBoxOffsetXMap[COPY_MAX_Z][COPY_MAX_X];
int CopyCollisionBoxOffsetZMap[COPY_MAX_Z][COPY_MAX_X];
int CopyCollisionBoxSizeXMap[COPY_MAX_Z][COPY_MAX_X];
int CopyCollisionBoxSizeZMap[COPY_MAX_Z][COPY_MAX_X];
// 2026-05-11: 範囲コピー/貼り付けでも辺単位の当たり判定を一緒に扱うため追加。
int CopyCollisionEdgeMap[COPY_MAX_Z][COPY_MAX_X];
// 2026-05-11: 範囲コピー/貼り付けでも辺の伸縮値を一緒に扱うため追加。
int CopyCollisionEdgeScaleMap[COPY_MAX_Z][COPY_MAX_X][4];
// 2026-05-11: 範囲コピー/貼り付けでも辺の厚さを一緒に扱うため追加。
int CopyCollisionEdgeThicknessMap[COPY_MAX_Z][COPY_MAX_X][4];
// 2026-05-13: 範囲コピー/貼り付けでもコーナー当たり調整値を一緒に扱うため追加。
int CopyCollisionCornerScaleMap[COPY_MAX_Z][COPY_MAX_X];
int CopyCollisionCornerThicknessMap[COPY_MAX_Z][COPY_MAX_X];
int CopyCollisionCornerOffsetMap[COPY_MAX_Z][COPY_MAX_X];

// 2026-05-13: コーナー当たり判定の奥行オフセットを編集・保存するため追加。
int CollisionCornerOffsetMap[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];

#pragma endregion


#pragma region ===== 共通判定 =====

static bool IsHoverValid()
{
    return IsMapPosValid(currentLayer, hoverZ, hoverX);
}
// 2026-07-08: 巡回ポイントを、選択中の敵配置セルへ紐づけるための共通処理。
static bool IsSelectedPatrolEnemyValid()
{
    return IsMapPosValid(selectedPatrolEnemyLayer, selectedPatrolEnemyZ, selectedPatrolEnemyX) &&
        EnemyMap[selectedPatrolEnemyLayer][selectedPatrolEnemyZ][selectedPatrolEnemyX] >= 0;
}

static void ClearEnemyPatrolPoints(int layer, int z, int x)
{
    if (!IsMapPosValid(layer, z, x))
        return;

    EnemyPatrolCountMap[layer][z][x] = 0;
    for (int i = 0; i < ENEMY_PATROL_POINT_MAX; i++)
    {
        EnemyPatrolXMap[layer][z][x][i] = -1;
        EnemyPatrolZMap[layer][z][x][i] = -1;
    }
}

static void SelectPatrolEnemy(int layer, int z, int x)
{
    if (!IsMapPosValid(layer, z, x) || EnemyMap[layer][z][x] < 0)
        return;

    selectedPatrolEnemyLayer = layer;
    selectedPatrolEnemyX = x;
    selectedPatrolEnemyZ = z;
}

static void AddPatrolPointToSelectedEnemy(int z, int x)
{
    if (!IsSelectedPatrolEnemyValid())
        return;

    int& count = EnemyPatrolCountMap[selectedPatrolEnemyLayer][selectedPatrolEnemyZ][selectedPatrolEnemyX];
    if (count >= ENEMY_PATROL_POINT_MAX)
        return;

    EnemyPatrolXMap[selectedPatrolEnemyLayer][selectedPatrolEnemyZ][selectedPatrolEnemyX][count] = x;
    EnemyPatrolZMap[selectedPatrolEnemyLayer][selectedPatrolEnemyZ][selectedPatrolEnemyX][count] = z;
    count++;
}

static bool RemovePatrolPointFromSelectedEnemy(int z, int x)
{
    if (!IsSelectedPatrolEnemyValid())
        return false;

    int& count = EnemyPatrolCountMap[selectedPatrolEnemyLayer][selectedPatrolEnemyZ][selectedPatrolEnemyX];
    for (int i = 0; i < count; i++)
    {
        if (EnemyPatrolXMap[selectedPatrolEnemyLayer][selectedPatrolEnemyZ][selectedPatrolEnemyX][i] == x &&
            EnemyPatrolZMap[selectedPatrolEnemyLayer][selectedPatrolEnemyZ][selectedPatrolEnemyX][i] == z)
        {
            for (int j = i; j < count - 1; j++)
            {
                EnemyPatrolXMap[selectedPatrolEnemyLayer][selectedPatrolEnemyZ][selectedPatrolEnemyX][j] = EnemyPatrolXMap[selectedPatrolEnemyLayer][selectedPatrolEnemyZ][selectedPatrolEnemyX][j + 1];
                EnemyPatrolZMap[selectedPatrolEnemyLayer][selectedPatrolEnemyZ][selectedPatrolEnemyX][j] = EnemyPatrolZMap[selectedPatrolEnemyLayer][selectedPatrolEnemyZ][selectedPatrolEnemyX][j + 1];
            }
            count--;
            EnemyPatrolXMap[selectedPatrolEnemyLayer][selectedPatrolEnemyZ][selectedPatrolEnemyX][count] = -1;
            EnemyPatrolZMap[selectedPatrolEnemyLayer][selectedPatrolEnemyZ][selectedPatrolEnemyX][count] = -1;
            return true;
        }
    }

    return false;
}
static bool HasValidSelection()
{
    return
        IsMapPosValid(selectLayer, selectStartZ, selectStartX) &&
        IsMapPosValid(selectLayer, selectEndZ, selectEndX);
}

static void ClearRangeSelectionRect()
{
    // 2026-07-22: コピー配置を置き終わったら、コピー元の青い範囲枠だけ消せるよう選択座標を初期化する。
    selecting = false;
    selectStartX = -1;
    selectStartZ = -1;
    selectEndX = -1;
    selectEndZ = -1;
    selectLayer = currentLayer;
}
static void ClearMultiSelection();

void ClearEditorRangeSelection()
{
    selectMode = false;
    pasteMode = false;
    ClearRangeSelectionRect();
    ClearMultiSelection();
}
#pragma endregion

#include "MapEditorCollisionEditing.inl"

#include "MapEditorSelectionClipboard.inl"

int GetEditorEventIdFromModelForUI(int modelId)
{
    // 2026-07-22: MapUI.cppからEVENTの表示名を出すため、model_config.csvのplaceId変換を公開する。
    return GetEditorEventIdFromModel(modelId);
}

#pragma region ===== 複数選択 =====

static void ClearMultiSelection()
{
    for (int y = 0; y < BLOCK_NUM_Y; y++)
    {
        for (int z = 0; z < BLOCK_NUM_Z; z++)
        {
            for (int x = 0; x < BLOCK_NUM_X; x++)
            {
                multiSelectMap[y][z][x] = false;
            }
        }
    }

    multiSelectCount = 0;
}

static void ToggleMultiSelectionCell(int layer, int z, int x)
{
    if (!IsMapPosValid(layer, z, x))
        return;

    multiSelectMap[layer][z][x] = !multiSelectMap[layer][z][x];
    multiSelectCount += multiSelectMap[layer][z][x] ? 1 : -1;
    if (multiSelectCount < 0)
        multiSelectCount = 0;
}

static void RotateCollisionEdgeValuesInCell(int layer, int z, int x)
{
    if (CollisionEdgeMap[layer][z][x] < 0)
        return;

    int oldMask = CollisionEdgeMap[layer][z][x];
    int oldScale[4];
    int oldThickness[4];
    for (int edge = 0; edge < 4; edge++)
    {
        oldScale[edge] = CollisionEdgeScaleMap[layer][z][x][edge];
        oldThickness[edge] = CollisionEdgeThicknessMap[layer][z][x][edge];
    }

    int newMask = 0;
    for (int edge = 0; edge < 4; edge++)
    {
        int rotatedEdge = (edge + 1) & 3;
        if (oldMask & GetCollisionEdgeBit(edge))
            newMask |= GetCollisionEdgeBit(rotatedEdge);

        CollisionEdgeScaleMap[layer][z][x][rotatedEdge] = oldScale[edge];
        CollisionEdgeThicknessMap[layer][z][x][rotatedEdge] = oldThickness[edge];
    }

    CollisionEdgeMap[layer][z][x] = newMask > 0 ? newMask : -1;
}

static void RotateCollisionBoxValuesInCell(int layer, int z, int x)
{
    int rotatedOffsetX = 0;
    int rotatedOffsetZ = 0;
    int rotatedSizeX = 0;
    int rotatedSizeZ = 0;
    RotateCopyBoxValues(
        CollisionBoxOffsetXMap[layer][z][x],
        CollisionBoxOffsetZMap[layer][z][x],
        CollisionBoxSizeXMap[layer][z][x],
        CollisionBoxSizeZMap[layer][z][x],
        1,
        rotatedOffsetX,
        rotatedOffsetZ,
        rotatedSizeX,
        rotatedSizeZ);

    CollisionBoxOffsetXMap[layer][z][x] = rotatedOffsetX;
    CollisionBoxOffsetZMap[layer][z][x] = rotatedOffsetZ;
    CollisionBoxSizeXMap[layer][z][x] = rotatedSizeX;
    CollisionBoxSizeZMap[layer][z][x] = rotatedSizeZ;
}

static void RotateCellDirectionOnly(int layer, int z, int x)
{
    FloorRot[layer][z][x] = (FloorRot[layer][z][x] + 1) & 3;
    WallRotA[layer][z][x] = (WallRotA[layer][z][x] + 1) & 3;
    WallRotB[layer][z][x] = (WallRotB[layer][z][x] + 1) & 3;
    CornerRot[layer][z][x] = (CornerRot[layer][z][x] + 1) & 3;
    DecoRot[layer][z][x] = (DecoRot[layer][z][x] + 1) & 3;

    RotateCollisionBoxValuesInCell(layer, z, x);
    RotateCollisionEdgeValuesInCell(layer, z, x);
}

static void RotateMultiSelectedCells()
{
    if (multiSelectCount <= 0)
        return;

    PushUndo();

    for (int y = 0; y < BLOCK_NUM_Y; y++)
    {
        for (int z = 0; z < BLOCK_NUM_Z; z++)
        {
            for (int x = 0; x < BLOCK_NUM_X; x++)
            {
                if (multiSelectMap[y][z][x])
                    RotateCellDirectionOnly(y, z, x);
            }
        }
    }

    MarkEditorMapDirty();
}

static void RotateRangeSelectedCells()
{
    if (!HasValidSelection())
        return;

    int minX = min(selectStartX, selectEndX);
    int maxX = max(selectStartX, selectEndX);
    int minZ = min(selectStartZ, selectEndZ);
    int maxZ = max(selectStartZ, selectEndZ);

    PushUndo();

    for (int z = minZ; z <= maxZ; z++)
    {
        for (int x = minX; x <= maxX; x++)
        {
            RotateCellDirectionOnly(selectLayer, z, x);
        }
    }

    MarkEditorMapDirty();
}
static bool EditorCellNeedsColorChange(int layer, int z, int x)
{
    if (!IsMapPosValid(layer, z, x))
        return false;

    return
        (FloorMap[layer][z][x] >= 0 && FloorColorMap[layer][z][x] != editorColorIndex) ||
        (WallMapA[layer][z][x] >= 0 && WallColorMapA[layer][z][x] != editorColorIndex) ||
        (WallMapB[layer][z][x] >= 0 && WallColorMapB[layer][z][x] != editorColorIndex) ||
        (CornerMap[layer][z][x] >= 0 && CornerColorMap[layer][z][x] != editorColorIndex) ||
        (DecoMap[layer][z][x] >= 0 && DecoColorMap[layer][z][x] != editorColorIndex);
}

static void ApplyEditorColorToCell(int layer, int z, int x)
{
    // 2026-07-21: 選択範囲の色変更は、実際に置いてあるモデルの色番号だけをまとめて更新する。
    if (FloorMap[layer][z][x] >= 0)
        FloorColorMap[layer][z][x] = editorColorIndex;
    if (WallMapA[layer][z][x] >= 0)
        WallColorMapA[layer][z][x] = editorColorIndex;
    if (WallMapB[layer][z][x] >= 0)
        WallColorMapB[layer][z][x] = editorColorIndex;
    if (CornerMap[layer][z][x] >= 0)
        CornerColorMap[layer][z][x] = editorColorIndex;
    if (DecoMap[layer][z][x] >= 0)
        DecoColorMap[layer][z][x] = editorColorIndex;
}

bool ApplyEditorColorToSelection()
{
    bool changed = false;
    bool pushedUndo = false;

    auto applySelectedCell = [&](int layer, int z, int x)
    {
        if (!EditorCellNeedsColorChange(layer, z, x))
            return;

        if (!pushedUndo)
        {
            PushUndo();
            pushedUndo = true;
        }

        ApplyEditorColorToCell(layer, z, x);
        changed = true;
    };

    // 2026-07-21: 範囲選択がある時は、ドラッグで囲った範囲を優先して一括で塗り替える。
    if (selectMode && HasValidSelection())
    {
        int minX = min(selectStartX, selectEndX);
        int maxX = max(selectStartX, selectEndX);
        int minZ = min(selectStartZ, selectEndZ);
        int maxZ = max(selectStartZ, selectEndZ);

        for (int z = minZ; z <= maxZ; z++)
        {
            for (int x = minX; x <= maxX; x++)
            {
                applySelectedCell(selectLayer, z, x);
            }
        }
    }
    else if (multiSelectCount > 0)
    {
        // 2026-07-21: Ctrl+左クリックで離して選んだマスも、色変更ボタンからまとめて塗り替えられるようにする。
        for (int layer = 0; layer < BLOCK_NUM_Y; layer++)
        {
            for (int z = 0; z < BLOCK_NUM_Z; z++)
            {
                for (int x = 0; x < BLOCK_NUM_X; x++)
                {
                    if (multiSelectMap[layer][z][x])
                        applySelectedCell(layer, z, x);
                }
            }
        }
    }

    if (changed)
        MarkEditorMapDirty();

    return changed;
}

#pragma endregion

#pragma region ===== 選択モデル =====

int GetSelectedModel()
{
    if (currentTab < 0 || currentTab >= TAB_MAX)
        return -1;

    // 2026-07-21: selectedModel == -1 は未選択状態。配置ではなく左ドラッグのカメラ移動に使う。
    if (selectedModel < 0 || selectedModel >= tabModelCount[currentTab])
        return -1;

    return tabModelList[currentTab][selectedModel];
}

bool FindEditorEventPosition(int eventId, int& layer, int& z, int& x)
{
    for (int y = 0; y < BLOCK_NUM_Y; y++)
    {
        for (int zz = 0; zz < BLOCK_NUM_Z; zz++)
        {
            for (int xx = 0; xx < BLOCK_NUM_X; xx++)
            {
                if (EventMap[y][zz][xx] == eventId)
                {
                    layer = y;
                    z = zz;
                    x = xx;
                    return true;
                }
            }
        }
    }

    layer = -1;
    z = -1;
    x = -1;
    return false;
}

bool SyncEditorStartGoalFromEvents()
{
    // 2026-07-16: START/GOAL未配置を初期座標と誤判定しないよう、EVENT配置の実在から座標を同期する。
    bool hasStart = FindEditorEventPosition(0, startY, startZ, startX);
    bool hasGoal = FindEditorEventPosition(1, goalY, goalZ, goalX);
    return hasStart && hasGoal;
}
static int GetEditorPlacementRot(int tab, int z, int x)
{
    return currentRot;
}

#pragma endregion


#pragma region ===== マップ初期化 =====

static bool ClearEditorCellAllKinds(int layer, int z, int x);
void ClearCurrentLayer()
{
    int layer = currentLayer;

    if (layer < 0 || layer >= BLOCK_NUM_Y)
        return;

    for (int z = 0; z < BLOCK_NUM_Z; z++)
    {
        for (int x = 0; x < BLOCK_NUM_X; x++)
        {
            ClearEditorCellAllKinds(layer, z, x);
        }
    }
}

void ResetAllMap()
{
    // 2026-05-13: マップ読込/リセット時に前の開始・ゴール座標が残らないよう追加。
    startX = -1; startY = -1; startZ = -1;
    goalX = -1; goalY = -1; goalZ = -1;
    selectedPatrolEnemyLayer = -1;
    selectedPatrolEnemyX = -1;
    selectedPatrolEnemyZ = -1;
    for (int y = 0; y < BLOCK_NUM_Y; y++)
    {
        for (int z = 0; z < BLOCK_NUM_Z; z++)
        {
            for (int x = 0; x < BLOCK_NUM_X; x++)
            {
                ClearEditorCellAllKinds(y, z, x);
            }
        }
    }
}

#pragma endregion



bool IsEditorMapDirty()
{
    return editorMapDirty;
}

void MarkEditorMapDirty()
{
    editorMapDirty = true;
}

void ClearEditorMapDirty()
{
    editorMapDirty = false;
}

static int GetEraserTargetFromTab(int tab)
{
    switch (tab)
    {
    case FLOOR:
        return ERASER_TARGET_FLOOR;

    case WALL:
        return ERASER_TARGET_WALL;

    case CORNER:
        return ERASER_TARGET_CORNER;

    case DECO:
        return ERASER_TARGET_DECO;

    case ENEMY:
        return ERASER_TARGET_ENEMY;

    case EVENT:
        return ERASER_TARGET_EVENT;

    default:
        return -1;
    }
}

// この削除操作を実行したら、本当に何か消えるか？だけを判定する関数
static bool CanClearEditorCellByTarget(int layer, int z, int x, int target)
{
    if (!IsMapPosValid(layer, z, x))
        return false;

    if (target == ERASER_TARGET_FLOOR)
    {
        return FloorMap[layer][z][x] >= 0;
    }
    else if (target == ERASER_TARGET_WALL)
    {
        int wallRot = GetEditorPlacementRot(WALL, z, x);

        return
            (WallMapA[layer][z][x] >= 0 &&
                WallRotA[layer][z][x] == wallRot) ||
            (WallMapB[layer][z][x] >= 0 &&
                WallRotB[layer][z][x] == wallRot);
    }
    else if (target == ERASER_TARGET_CORNER)
    {
        return CornerMap[layer][z][x] >= 0;
    }
    else if (target == ERASER_TARGET_DECO)
    {
        return DecoMap[layer][z][x] >= 0;
    }
    else if (target == ERASER_TARGET_ENEMY)
    {
        return EnemyMap[layer][z][x] >= 0;
    }
    else if (target == ERASER_TARGET_EVENT)
    {
        return EventMap[layer][z][x] >= 0;
    }

    return false;
}

static bool ClearEditorCellByTarget(int layer, int z, int x, int target)
{
    if (!CanClearEditorCellByTarget(layer, z, x, target))
        return false;

    if (target == ERASER_TARGET_FLOOR)
    {
        FloorMap[layer][z][x] = -1;
        FloorRot[layer][z][x] = 0;
        FloorColorMap[layer][z][x] = 0;

        return true;
    }
    else if (target == ERASER_TARGET_WALL)
    {
        int wallRot = GetEditorPlacementRot(WALL, z, x);

        if (WallMapA[layer][z][x] >= 0 && WallRotA[layer][z][x] == wallRot)
        {
            WallMapA[layer][z][x] = -1;
            WallRotA[layer][z][x] = 0;
            WallColorMapA[layer][z][x] = 0;
        }

        if (WallMapB[layer][z][x] >= 0 && WallRotB[layer][z][x] == wallRot)
        {
            WallMapB[layer][z][x] = -1;
            WallRotB[layer][z][x] = 0;
            WallColorMapB[layer][z][x] = 0;
        }

        return true;
    }
    else if (target == ERASER_TARGET_CORNER)
    {
        CornerMap[layer][z][x] = -1;
        CornerRot[layer][z][x] = 0;
        CornerColorMap[layer][z][x] = 0;
        CollisionCornerScaleMap[layer][z][x] = 100;
        CollisionCornerThicknessMap[layer][z][x] = 100;
        CollisionCornerOffsetMap[layer][z][x] = 0;

        return true;
    }
    else if (target == ERASER_TARGET_DECO)
    {
        DecoMap[layer][z][x] = -1;
        DecoRot[layer][z][x] = 0;
        DecoColorMap[layer][z][x] = 0;
        ClimbLinkMap[layer][z][x] = 0;

        return true;
    }
    else if (target == ERASER_TARGET_ENEMY)
    {
        EnemyMap[layer][z][x] = -1;
        ClearEnemyPatrolPoints(layer, z, x);

        if (selectedPatrolEnemyLayer == layer &&
            selectedPatrolEnemyX == x &&
            selectedPatrolEnemyZ == z)
        {
            selectedPatrolEnemyLayer = -1;
            selectedPatrolEnemyX = -1;
            selectedPatrolEnemyZ = -1;
        }

        return true;
    }
    else if (target == ERASER_TARGET_EVENT)
    {
        if (EventMap[layer][z][x] == 0)
        {
            startX = -1;
            startY = -1;
            startZ = -1;
        }
        else if (EventMap[layer][z][x] == 1)
        {
            goalX = -1;
            goalY = -1;
            goalZ = -1;
        }

        EventMap[layer][z][x] = -1;
        EventRot[layer][z][x] = 0;

        return true;
    }
    return false;
}

static bool TryClearEditorCellByTarget(int layer, int z, int x, int target)
{
    if (!CanClearEditorCellByTarget(layer, z, x, target))
        return false;

    PushUndo();

    return ClearEditorCellByTarget(layer, z, x, target);
}

static bool HasEditorCellContent(int layer, int z, int x)
{
    if (!IsMapPosValid(layer, z, x))
        return false;

    return
        FloorMap[layer][z][x] >= 0 ||
        WallMapA[layer][z][x] >= 0 ||
        WallMapB[layer][z][x] >= 0 ||
        CornerMap[layer][z][x] >= 0 ||
        DecoMap[layer][z][x] >= 0 ||
        EnemyMap[layer][z][x] >= 0 ||
        EventMap[layer][z][x] >= 0 ||
        CollisionMap[layer][z][x] >= 0 ||
        CollisionEdgeMap[layer][z][x] >= 0;
}

static bool ClearEditorCellAllKinds(int layer, int z, int x)
{
    // 2026-07-22: 消しゴムは現在選択中のタブに縛らず、同じマスの配置物をまとめて消せるようにする。
    if (!IsMapPosValid(layer, z, x))
        return false;

    bool changed = HasEditorCellContent(layer, z, x);

    if (EventMap[layer][z][x] == 0)
    {
        startX = -1;
        startY = -1;
        startZ = -1;
    }
    else if (EventMap[layer][z][x] == 1)
    {
        goalX = -1;
        goalY = -1;
        goalZ = -1;
    }

    FloorMap[layer][z][x] = -1;
    FloorRot[layer][z][x] = 0;
    FloorColorMap[layer][z][x] = 0;

    WallMapA[layer][z][x] = -1;
    WallRotA[layer][z][x] = 0;
    WallColorMapA[layer][z][x] = 0;
    WallMapB[layer][z][x] = -1;
    WallRotB[layer][z][x] = 0;
    WallColorMapB[layer][z][x] = 0;

    CornerMap[layer][z][x] = -1;
    CornerRot[layer][z][x] = 0;
    CornerColorMap[layer][z][x] = 0;
    CollisionCornerScaleMap[layer][z][x] = 100;
    CollisionCornerThicknessMap[layer][z][x] = 100;
    CollisionCornerOffsetMap[layer][z][x] = 0;

    DecoMap[layer][z][x] = -1;
    DecoRot[layer][z][x] = 0;
    DecoColorMap[layer][z][x] = 0;
    ClimbLinkMap[layer][z][x] = 0;

    EnemyMap[layer][z][x] = -1;
    ClearEnemyPatrolPoints(layer, z, x);
    if (selectedPatrolEnemyLayer == layer && selectedPatrolEnemyX == x && selectedPatrolEnemyZ == z)
    {
        selectedPatrolEnemyLayer = -1;
        selectedPatrolEnemyX = -1;
        selectedPatrolEnemyZ = -1;
    }

    EventMap[layer][z][x] = -1;
    EventRot[layer][z][x] = 0;

    CollisionMap[layer][z][x] = -1;
    ResetCollisionBox(layer, z, x);
    CollisionEdgeMap[layer][z][x] = -1;
    for (int edge = 0; edge < 4; edge++)
    {
        CollisionEdgeScaleMap[layer][z][x][edge] = 100;
        CollisionEdgeThicknessMap[layer][z][x][edge] = 100;
    }

    return changed;
}

static bool TryClearEditorCellAllKinds(int layer, int z, int x)
{
    if (!HasEditorCellContent(layer, z, x))
        return false;

    PushUndo();

    return ClearEditorCellAllKinds(layer, z, x);
}

static void ClearPendingEditorLoad()
{
    pendingLoadMapIndex = -1;
    PendingLoadMapName[0] = '\0';
    PendingLoadMapListIndex = -1;
}

static void LoadEditorMapNow(int mapIndex)
{
    currentMapIndex = mapIndex;
    gameCurrentMapName[0] = '\0';
    LoadMap(mapIndex);
    ClearPendingEditorLoad();
}

static void LoadEditorMapNow(const char* mapName)
{
    strcpy_s(gameCurrentMapName, 64, mapName);
    LoadMapByName(gameCurrentMapName);
    ClearPendingEditorLoad();
}

static void StartLoadMapConfirmForIndex(int mapIndex)
{
    pendingLoadMapIndex = mapIndex;
    PendingLoadMapName[0] = '\0';
    PendingLoadMapListIndex = -1;
    loadMapConfirmActive = true;
}

static void StartLoadMapConfirmForName(const char* mapName, int listIndex)
{
    pendingLoadMapIndex = -1;
    strcpy_s(PendingLoadMapName, sizeof(PendingLoadMapName), mapName);
    PendingLoadMapListIndex = listIndex;
    loadMapConfirmActive = true;
}

static void RequestEditorLoadMap(int mapIndex)
{
    // 2026-06-12: 未保存変更がある時は、読み込みで編集内容が消える前に確認するため追加。
    if (IsEditorMapDirty())
    {
        StartLoadMapConfirmForIndex(mapIndex);
        return;
    }

    LoadEditorMapNow(mapIndex);
}

static void RequestEditorLoadMap(const char* mapName)
{
    // 2026-06-12: F9などで同じCSVを読み直す時も、未保存なら破棄確認を出すため追加。
    if (IsEditorMapDirty())
    {
        StartLoadMapConfirmForName(mapName, -1);
        return;
    }

    LoadEditorMapNow(mapName);
}

static void RequestEditorLoadMapFromList(int listIndex)
{
    if (listIndex < 0 || listIndex >= mapNameCount)
        return;

    // 2026-06-12: 左右キーのマップ切替で、確認をキャンセルした時に表示中のマップ名を戻すため追加。
    if (IsEditorMapDirty())
    {
        StartLoadMapConfirmForName(mapNameList[listIndex], listIndex);
        return;
    }

    selectedMapListIndex = listIndex;
    strcpy_s(selectedMapName, sizeof(selectedMapName), mapNameList[selectedMapListIndex]);
    LoadEditorMapNow(selectedMapName);
}

static void UpdateLoadMapConfirm()
{
    if (Input::IsKeyTrigger(KEY_INPUT_Y))
    {
        ClearEditorMapDirty();
        loadMapConfirmActive = false;

        if (PendingLoadMapName[0] != '\0')
        {
            if (PendingLoadMapListIndex >= 0)
            {
                selectedMapListIndex = PendingLoadMapListIndex;
                strcpy_s(selectedMapName, sizeof(selectedMapName), mapNameList[selectedMapListIndex]);
            }
            LoadEditorMapNow(PendingLoadMapName);
        }
        else if (pendingLoadMapIndex >= 0)
        {
            LoadEditorMapNow(pendingLoadMapIndex);
        }
        else
        {
            ClearPendingEditorLoad();
        }
    }
    else if (Input::IsKeyTrigger(KEY_INPUT_N) || Input::IsKeyTrigger(KEY_INPUT_C) || Input::IsKeyTrigger(KEY_INPUT_F9))
    {
        loadMapConfirmActive = false;
        ClearPendingEditorLoad();
    }
}

static void DrawLoadMapConfirm()
{
    if (!loadMapConfirmActive)
        return;

    DrawBox(300, 260, 980, 400, GetColor(10, 10, 10), TRUE);
    DrawBox(300, 260, 980, 400, GetColor(255, 255, 255), FALSE);
    DrawString(330, 292, "保存されてません。保存せずマップを切り替えますか？", GetColor(255, 255, 0));
    DrawString(330, 330, "Y: 保存せず切替   N/C/F9: キャンセル", GetColor(220, 240, 255));
}
static bool IsEditorConfigCsv(const char* name)
{
    return
        strcmp(name, "model_config") == 0 ||
        strcmp(name, "collision_config") == 0 ||
        strcmp(name, "kit") == 0 ||
        strcmp(name, "kitbase") == 0 ||
        strcmp(name, "キット") == 0 ||
        strcmp(name, "\x83" "L\x83" "b\x83" "g") == 0;
}

static void CopyCsvBaseName(char* dst, size_t dstSize, const char* fileName)
{
    strcpy_s(dst, dstSize, fileName);
    char* dot = strrchr(dst, '.');
    if (dot != nullptr)
        *dot = '\0';
}

static void SanitizeKitFileName(char* text)
{
    for (int i = 0; text[i] != '\0'; i++)
    {
        char c = text[i];
        bool ok =
            (c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') ||
            c == '_' || c == '-';
        if (!ok)
            text[i] = '_';
    }
}

static void EnsureEditorKitDirectory()
{
    CreateDirectoryA("kits", NULL);
}

static void BuildEditorKitPath(char* dst, size_t dstSize, const char* kitName)
{
    sprintf_s(dst, dstSize, "kits\\%s.kit", kitName);
}

static void BuildEditorKitThumbnailPath(char* dst, size_t dstSize, const char* kitName)
{
    sprintf_s(dst, dstSize, "kits\\%s.bmp", kitName);
}

static const char* GetEditorKitCsvPath()
{
    // 2026-06-26: ベースキットを個別.kitではなく一つのCSVから増やせるよう、参照先を共通化する。
    return "\x83" "L\x83" "b\x83" "g.csv";
}

static const char* GetEditorKitBaseCsvPath()
{
    // 2026-06-26: エディタ内のボタンからベースキットを再生成できるよう、元CSVの参照先を共通化する。
    return "kitbase.csv";
}

static bool IsEditorKitNameListed(const char* kitName)
{
    for (int i = 0; i < editorKitCount; i++)
    {
        if (strcmp(editorKitNameList[i], kitName) == 0)
            return true;
    }

    return false;
}

void RefreshEditorKitList()
{
    // 2026-06-25: 登録済みキットをkitsフォルダから一覧化し、左のキットタブへ名前表示するため追加。
    EnsureEditorKitDirectory();
    static bool thumbnailHandlesInitialized = false;
    if (!thumbnailHandlesInitialized)
    {
        for (int i = 0; i < EDITOR_KIT_LIST_MAX; i++)
        {
            editorKitThumbnailHandle[i] = -1;
            editorKitFromCsv[i] = false;
        }
        thumbnailHandlesInitialized = true;
    }

    for (int i = 0; i < EDITOR_KIT_LIST_MAX; i++)
    {
        if (editorKitThumbnailHandle[i] != -1)
        {
            DeleteGraph(editorKitThumbnailHandle[i]);
            editorKitThumbnailHandle[i] = -1;
        }
        editorKitFromCsv[i] = false;
    }
    editorKitCount = 0;

    ifstream kitCsv(GetEditorKitCsvPath());
    if (kitCsv.is_open())
    {
        string line;
        while (editorKitCount < EDITOR_KIT_LIST_MAX && getline(kitCsv, line))
        {
            vector<string> cols = SplitCSV(line);
            if (cols.size() >= 2 && cols[0] == "KIT")
            {
                strcpy_s(editorKitNameList[editorKitCount], sizeof(editorKitNameList[editorKitCount]), cols[1].c_str());

                char thumbPath[MAX_PATH];
                BuildEditorKitThumbnailPath(thumbPath, sizeof(thumbPath), editorKitNameList[editorKitCount]);
                editorKitThumbnailHandle[editorKitCount] = LoadGraph(thumbPath);
                editorKitFromCsv[editorKitCount] = true;

                editorKitCount++;
            }
        }
    }

    WIN32_FIND_DATAA findData;
    HANDLE handle = FindFirstFileA("kits\\*.kit", &findData);
    if (handle != INVALID_HANDLE_VALUE)
    {
        do
        {
            if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
                continue;

            char baseName[64];
            CopyCsvBaseName(baseName, sizeof(baseName), findData.cFileName);
            if (IsEditorKitNameListed(baseName))
                continue;

            strcpy_s(editorKitNameList[editorKitCount], sizeof(editorKitNameList[editorKitCount]), baseName);

            char thumbPath[MAX_PATH];
            BuildEditorKitThumbnailPath(thumbPath, sizeof(thumbPath), baseName);
            editorKitThumbnailHandle[editorKitCount] = LoadGraph(thumbPath);
            editorKitFromCsv[editorKitCount] = false;

            editorKitCount++;
        } while (editorKitCount < EDITOR_KIT_LIST_MAX && FindNextFileA(handle, &findData));

        FindClose(handle);
    }

    if (selectedEditorKitIndex >= editorKitCount)
        selectedEditorKitIndex = editorKitCount - 1;
}

int GetEditorKitCount()
{
    return editorKitCount;
}

const char* GetEditorKitName(int index)
{
    if (index < 0 || index >= editorKitCount)
        return "";
    return editorKitNameList[index];
}

int GetSelectedEditorKitIndex()
{
    return selectedEditorKitIndex;
}

int GetEditorKitThumbnailHandle(int index)
{
    if (index < 0 || index >= editorKitCount)
        return -1;
    return editorKitThumbnailHandle[index];
}

bool IsEditorKitFromCsv(int index)
{
    // 2026-06-26: キットUIで「ベース」「自作」を切り替える判定に使う。
    if (index < 0 || index >= editorKitCount)
        return false;
    return editorKitFromCsv[index];
}

struct EditorBaseKitCell
{
    int floor;
    int floorRot;
    int wallA;
    int wallB;
    int wallRotA;
    int wallRotB;
    int corner;
    int cornerRot;
    int deco;
    int decoRot;
    int eventId;
    int collision;
    int offsetX;
    int offsetZ;
    int sizeX;
    int sizeZ;
    int collisionEdge;
    int edgeScale[4];
    int edgeThickness[4];
    int cornerScale;
    int cornerThickness;
    int cornerOffset;
};

struct EditorBaseKitDef
{
    string name;
    int layer;
    int z;
    int x;
    int sizeX;
    int sizeZ;
};

static void ResetEditorBaseKitCell(EditorBaseKitCell& cell)
{
    cell.floor = -1;
    cell.floorRot = 0;
    cell.wallA = -1;
    cell.wallB = -1;
    cell.wallRotA = 0;
    cell.wallRotB = 0;
    cell.corner = -1;
    cell.cornerRot = 0;
    cell.deco = -1;
    cell.decoRot = 0;
    cell.eventId = -1;
    cell.collision = -1;
    cell.offsetX = 0;
    cell.offsetZ = 0;
    cell.sizeX = (int)BLOCK_SIZE;
    cell.sizeZ = (int)BLOCK_SIZE;
    cell.collisionEdge = -1;
    for (int edge = 0; edge < 4; edge++)
    {
        cell.edgeScale[edge] = 100;
        cell.edgeThickness[edge] = 100;
    }
    cell.cornerScale = 100;
    cell.cornerThickness = 100;
    cell.cornerOffset = 0;
}

bool RebuildEditorBaseKitCsv()
{
    // 2026-06-26: kitbase.csvを編集した後、エディタ上の操作だけでキット.csvへ反映できるよう追加。
    static EditorBaseKitCell cells[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];
    static bool used[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];
    vector<EditorBaseKitDef> kitDefs;

    for (int y = 0; y < BLOCK_NUM_Y; y++)
    {
        for (int z = 0; z < BLOCK_NUM_Z; z++)
        {
            for (int x = 0; x < BLOCK_NUM_X; x++)
            {
                ResetEditorBaseKitCell(cells[y][z][x]);
                used[y][z][x] = false;
            }
        }
    }

    ifstream ifs(GetEditorKitBaseCsvPath());
    if (!ifs.is_open())
        return false;

    string section;
    string line;
    while (getline(ifs, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        if (line[0] == '[')
        {
            section = line;
            continue;
        }

        vector<string> cols = SplitCSV(line);
        if (cols.empty() || cols[0] == "y" || cols[0] == "NAME" || cols[0] == "SIZE" || cols[0] == "START" || cols[0] == "GOAL")
            continue;

        if (section == "[KIT]")
        {
            // 2026-06-28: 「3x3部屋」など用途名でベースキットを作れるよう、kitbase.csv内の範囲指定を読む。
            if (cols[0] == "name" || cols.size() < 6)
                continue;

            EditorBaseKitDef def;
            def.name = cols[0];
            def.layer = stoi(cols[1]);
            def.z = stoi(cols[2]);
            def.x = stoi(cols[3]);
            def.sizeX = stoi(cols[4]);
            def.sizeZ = stoi(cols[5]);
            if (!def.name.empty() && def.sizeX > 0 && def.sizeZ > 0)
                kitDefs.push_back(def);
            continue;
        }

        if (cols.size() < 4)
            continue;

        int y = stoi(cols[0]);
        int z = stoi(cols[1]);
        int x = stoi(cols[2]);
        if (!IsMapPosValid(y, z, x))
            continue;

        EditorBaseKitCell& cell = cells[y][z][x];
        used[y][z][x] = true;

        if (section == "[FLOOR]" && cols.size() >= 5)
        {
            cell.floor = stoi(cols[3]);
            cell.floorRot = stoi(cols[4]);
        }
        else if (section == "[WALL_A]" && cols.size() >= 5)
        {
            cell.wallA = stoi(cols[3]);
            cell.wallRotA = stoi(cols[4]);
        }
        else if (section == "[WALL_B]" && cols.size() >= 5)
        {
            cell.wallB = stoi(cols[3]);
            cell.wallRotB = stoi(cols[4]);
        }
        else if (section == "[CORNER]" && cols.size() >= 5)
        {
            cell.corner = stoi(cols[3]);
            cell.cornerRot = stoi(cols[4]);
            cell.cornerScale = cols.size() > 5 ? stoi(cols[5]) : 100;
            cell.cornerThickness = cols.size() > 6 ? stoi(cols[6]) : 100;
            cell.cornerOffset = cols.size() > 7 ? stoi(cols[7]) : 0;
        }
        else if (section == "[DECO]" && cols.size() >= 5)
        {
            cell.deco = stoi(cols[3]);
            cell.decoRot = stoi(cols[4]);
        }
        else if (section == "[COLLISION]" && cols.size() >= 4)
        {
            cell.collision = stoi(cols[3]);
            cell.offsetX = cols.size() > 4 ? stoi(cols[4]) : 0;
            cell.offsetZ = cols.size() > 5 ? stoi(cols[5]) : 0;
            cell.sizeX = cols.size() > 6 ? stoi(cols[6]) : (int)BLOCK_SIZE;
            cell.sizeZ = cols.size() > 7 ? stoi(cols[7]) : (int)BLOCK_SIZE;
        }
        else if (section == "[COLLISION_EDGE]" && cols.size() >= 4)
        {
            cell.collisionEdge = stoi(cols[3]);
            for (int edge = 0; edge < 4; edge++)
            {
                cell.edgeScale[edge] = cols.size() > 4 + edge ? stoi(cols[4 + edge]) : 100;
                cell.edgeThickness[edge] = cols.size() > 8 + edge ? stoi(cols[8 + edge]) : 100;
            }
        }
        else if (section == "[EVENT]" && cols.size() >= 4)
        {
            cell.eventId = stoi(cols[3]);
        }
    }

    ofstream ofs(GetEditorKitCsvPath());
    if (!ofs.is_open())
        return false;

    ofs << "#KIT_CSV\n";
    ofs << "# generated from kitbase.csv\n";

    auto writeKitRange = [&](const char* kitName, int layer, int minZ, int minX, int sizeX, int sizeZ)
    {
        if (layer < 0 || layer >= BLOCK_NUM_Y || minZ < 0 || minX < 0 || sizeX <= 0 || sizeZ <= 0)
            return;

        int maxZ = min(BLOCK_NUM_Z - 1, minZ + sizeZ - 1);
        int maxX = min(BLOCK_NUM_X - 1, minX + sizeX - 1);
        if (maxZ < minZ || maxX < minX)
            return;

        ofs << "KIT," << kitName << "," << (maxX - minX + 1) << "," << (maxZ - minZ + 1) << "\n";
        for (int z = minZ; z <= maxZ; z++)
        {
            for (int x = minX; x <= maxX; x++)
            {
                EditorBaseKitCell& cell = cells[layer][z][x];
                ofs << "CELL," << (z - minZ) << "," << (x - minX) << ","
                    << cell.floor << "," << cell.floorRot << ","
                    << cell.wallA << "," << cell.wallB << ","
                    << cell.wallRotA << "," << cell.wallRotB << ","
                    << cell.corner << "," << cell.cornerRot << ","
                    << cell.deco << "," << cell.decoRot << ","
                    << cell.eventId << ","
                    << cell.collision << ","
                    << cell.offsetX << "," << cell.offsetZ << ","
                    << cell.sizeX << "," << cell.sizeZ << ","
                    << cell.collisionEdge;

                for (int edge = 0; edge < 4; edge++)
                    ofs << "," << cell.edgeScale[edge];
                for (int edge = 0; edge < 4; edge++)
                    ofs << "," << cell.edgeThickness[edge];

                ofs << ","
                    << cell.cornerScale << ","
                    << cell.cornerThickness << ","
                    << cell.cornerOffset << "\n";
            }
        }
        ofs << "\n";
    };

    if (!kitDefs.empty())
    {
        for (int i = 0; i < (int)kitDefs.size(); i++)
        {
            const EditorBaseKitDef& def = kitDefs[i];
            writeKitRange(def.name.c_str(), def.layer, def.z, def.x, def.sizeX, def.sizeZ);
        }

        RefreshEditorKitList();
        return true;
    }

    for (int y = 0; y < BLOCK_NUM_Y; y++)
    {
        int minX = BLOCK_NUM_X;
        int maxX = -1;
        int minZ = BLOCK_NUM_Z;
        int maxZ = -1;

        for (int z = 0; z < BLOCK_NUM_Z; z++)
        {
            for (int x = 0; x < BLOCK_NUM_X; x++)
            {
                if (!used[y][z][x])
                    continue;

                minX = min(minX, x);
                maxX = max(maxX, x);
                minZ = min(minZ, z);
                maxZ = max(maxZ, z);
            }
        }

        if (maxX < minX || maxZ < minZ)
            continue;

        char kitName[64];
        sprintf_s(kitName, sizeof(kitName), "kitbase_y%d", y);
        writeKitRange(kitName, y, minZ, minX, maxX - minX + 1, maxZ - minZ + 1);
    }

    RefreshEditorKitList();
    return true;
}

static bool SaveDrawScreenReplaceFile(int x1, int y1, int x2, int y2, const char* path)
{
    if (path == nullptr || path[0] == '\0')
        return false;

    char tempPath[MAX_PATH];
    sprintf_s(tempPath, sizeof(tempPath), "%s.new.bmp", path);
    DeleteFileA(tempPath);

    if (SaveDrawScreen(x1, y1, x2, y2, tempPath) != 0)
    {
        DeleteFileA(tempPath);
        return false;
    }

    if (!MoveFileExA(tempPath, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    {
        DeleteFileA(tempPath);
        return false;
    }

    return true;
}

static void ReloadEditorKitThumbnailHandle(int index)
{
    if (index < 0 || index >= editorKitCount)
        return;

    if (editorKitThumbnailHandle[index] != -1)
    {
        DeleteGraph(editorKitThumbnailHandle[index]);
        editorKitThumbnailHandle[index] = -1;
    }

    char thumbPath[MAX_PATH];
    BuildEditorKitThumbnailPath(thumbPath, sizeof(thumbPath), editorKitNameList[index]);
    editorKitThumbnailHandle[index] = LoadGraph(thumbPath);
}
static bool GetSelectionScreenRectForThumbnail(int& x1, int& y1, int& x2, int& y2)
{
    if (!HasValidSelection())
        return false;

    int minX = min(selectStartX, selectEndX);
    int maxX = max(selectStartX, selectEndX) + 1;
    int minZ = min(selectStartZ, selectEndZ);
    int maxZ = max(selectStartZ, selectEndZ) + 1;
    float baseY = selectLayer * BLOCK_SIZE;
    float topY = baseY + BLOCK_SIZE * 0.9f;

    VECTOR points[8] =
    {
        VGet(minX * BLOCK_SIZE, baseY, minZ * BLOCK_SIZE),
        VGet(maxX * BLOCK_SIZE, baseY, minZ * BLOCK_SIZE),
        VGet(minX * BLOCK_SIZE, baseY, maxZ * BLOCK_SIZE),
        VGet(maxX * BLOCK_SIZE, baseY, maxZ * BLOCK_SIZE),
        VGet(minX * BLOCK_SIZE, topY, minZ * BLOCK_SIZE),
        VGet(maxX * BLOCK_SIZE, topY, minZ * BLOCK_SIZE),
        VGet(minX * BLOCK_SIZE, topY, maxZ * BLOCK_SIZE),
        VGet(maxX * BLOCK_SIZE, topY, maxZ * BLOCK_SIZE)
    };

    bool found = false;
    int minScreenX = SCREEN_W;
    int minScreenY = SCREEN_H;
    int maxScreenX = 0;
    int maxScreenY = 0;

    for (int i = 0; i < 8; i++)
    {
        VECTOR screen = ConvWorldPosToScreenPos(points[i]);
        if (screen.z < 0.0f || screen.z > 1.0f)
            continue;

        minScreenX = min(minScreenX, (int)screen.x);
        minScreenY = min(minScreenY, (int)screen.y);
        maxScreenX = max(maxScreenX, (int)screen.x);
        maxScreenY = max(maxScreenY, (int)screen.y);
        found = true;
    }

    if (!found)
        return false;

    const int pad = 28;
    x1 = max(0, minScreenX - pad);
    y1 = max(0, minScreenY - pad);
    x2 = min(SCREEN_W - 1, maxScreenX + pad);
    y2 = min(SCREEN_H - 1, maxScreenY + pad);

    return x2 > x1 && y2 > y1;
}

static void RequestEditorKitThumbnailSave(const char* kitName)
{
    // 2026-06-25: キット登録時、UIなしの3D描画から選択範囲部分だけを切り抜いてサムネ保存するため追加。
    strcpy_s(pendingKitThumbnailName, sizeof(pendingKitThumbnailName), kitName);
    pendingKitThumbnailSave = true;
}

static void DrawEditorKitThumbnailModel(int tab, int id, int x, int y, int z, int rot)
{
    if (id < 0 || id >= MODEL_MAX || modelHandles[id] == -1)
        return;

    VECTOR pos = GetModelDrawPosition(tab, x, y, z, rot);
    pos = ApplyEditorModelConfigOffset(tab, id, pos);
    int drawRot = GetEditorModelDrawRot(tab, id, rot);

    MV1SetPosition(modelHandles[id], pos);
    MV1SetRotationXYZ(modelHandles[id], VGet(0.0f, RotToRad(drawRot), 0.0f));
    MV1SetOpacityRate(modelHandles[id], 1.0f);
    // 2026-07-21: 保存サムネイルでもモデルが角度で暗くならないよう、描画中だけライト計算を切る。
    int oldLighting = GetLightEnable();
    SetUseLighting(FALSE);
    SetLightEnable(FALSE);
    MV1DrawModel(modelHandles[id]);
    SetLightEnable(oldLighting);
    SetUseLighting(TRUE);
}

bool SaveEditorMapThumbnail(const char* thumbnailPath)
{
    // 2026-07-17: マップ一覧用サムネを現在画面の切り抜きではなく、専用のオフスクリーン描画で安定して保存する。
    if (thumbnailPath == nullptr || thumbnailPath[0] == '\0')
        return false;

    int layer = startY;
    if (layer < 0 || layer >= BLOCK_NUM_Y)
        layer = currentLayer;
    if (layer < 0 || layer >= BLOCK_NUM_Y)
        layer = 0;

    int minX = BLOCK_NUM_X;
    int minZ = BLOCK_NUM_Z;
    int maxX = -1;
    int maxZ = -1;

    auto IncludeThumbnailCell = [&](int x, int z)
    {
        if (x < minX) minX = x;
        if (z < minZ) minZ = z;
        if (x > maxX) maxX = x;
        if (z > maxZ) maxZ = z;
    };

    for (int z = 0; z < BLOCK_NUM_Z; z++)
    {
        for (int x = 0; x < BLOCK_NUM_X; x++)
        {
            if (FloorMap[layer][z][x] >= 0 ||
                WallMapA[layer][z][x] >= 0 ||
                WallMapB[layer][z][x] >= 0 ||
                CornerMap[layer][z][x] >= 0 ||
                DecoMap[layer][z][x] >= 0 ||
                EventMap[layer][z][x] >= 0 ||
                EnemyMap[layer][z][x] >= 0)
            {
                IncludeThumbnailCell(x, z);
            }
        }
    }

    // 2026-07-17: 空のマップでもサムネ生成関数を呼べるよう、範囲が無い時は全体を写す。
    if (maxX < minX || maxZ < minZ)
    {
        minX = 0;
        minZ = 0;
        maxX = BLOCK_NUM_X - 1;
        maxZ = BLOCK_NUM_Z - 1;
    }

    const int thumbnailSize = 512;
    int thumbnailScreen = MakeScreen(thumbnailSize, thumbnailSize, TRUE);
    if (thumbnailScreen == -1)
        return false;

    int oldDrawScreen = GetDrawScreen();
    SetDrawScreen(thumbnailScreen);
    ClearDrawScreen();

    float centerX = (minX + maxX + 1) * BLOCK_SIZE * 0.5f;
    float centerZ = (minZ + maxZ + 1) * BLOCK_SIZE * 0.5f;
    float centerY = layer * BLOCK_SIZE + BLOCK_SIZE * 0.25f;
    float rangeX = (maxX - minX + 1) * BLOCK_SIZE;
    float rangeZ = (maxZ - minZ + 1) * BLOCK_SIZE;
    // 2026-07-17: サムネの見え方を毎回そろえるため、開始階層を固定角度・固定距離のカメラで撮る。
    float cameraDist = max(900.0f, max(rangeX, rangeZ) * 1.28f);
    float cameraRotY = 0.78f;
    float cameraRotX = 0.58f;

    SetUseZBuffer3D(TRUE);
    SetWriteZBuffer3D(TRUE);
    SetCameraNearFar(10.0f, 50000.0f);
    SetCameraPositionAndTarget_UpVecY(
        VGet(
            centerX + sinf(cameraRotY) * cosf(cameraRotX) * cameraDist,
            centerY + 280.0f + sinf(cameraRotX) * cameraDist,
            centerZ + cosf(cameraRotY) * cosf(cameraRotX) * cameraDist),
        VGet(centerX, centerY, centerZ));

    auto DrawThumbnailFloorBlockBase = [](int x, int y, int z, float topOffset, float height, int faceColor, int edgeColor)
    {
        float left = x * BLOCK_SIZE;
        float right = (x + 1) * BLOCK_SIZE;
        float front = z * BLOCK_SIZE;
        float back = (z + 1) * BLOCK_SIZE;
        float topY = y * BLOCK_SIZE + topOffset - 2.0f;
        float bottomY = topY - height;

        DrawCube3D(
            VGet(left, bottomY, front),
            VGet(right, topY, back),
            faceColor,
            edgeColor,
            TRUE
        );
    };

    for (int z = minZ; z <= maxZ; z++)
    {
        for (int x = minX; x <= maxX; x++)
        {
            if (FloorMap[layer][z][x] >= 0)
                DrawThumbnailFloorBlockBase(x, layer, z, 0.0f, BLOCK_SIZE * 0.22f, GetColor(82, 72, 58), GetColor(38, 32, 26));

            if (DecoMap[layer][z][x] == 6)
                DrawThumbnailFloorBlockBase(x, layer, z, BLOCK_SIZE * 0.5f, BLOCK_SIZE * 0.5f, GetColor(90, 80, 62), GetColor(42, 35, 27));

            DrawEditorKitThumbnailModel(FLOOR, FloorMap[layer][z][x], x, layer, z, FloorRot[layer][z][x]);
            DrawEditorKitThumbnailModel(WALL, WallMapA[layer][z][x], x, layer, z, WallRotA[layer][z][x]);
            DrawEditorKitThumbnailModel(WALL, WallMapB[layer][z][x], x, layer, z, WallRotB[layer][z][x]);
            DrawEditorKitThumbnailModel(CORNER, CornerMap[layer][z][x], x, layer, z, CornerRot[layer][z][x]);
            DrawEditorKitThumbnailModel(DECO, DecoMap[layer][z][x], x, layer, z, DecoRot[layer][z][x]);
        }
    }

    int saveResult = SaveDrawScreen(0, 0, thumbnailSize - 1, thumbnailSize - 1, thumbnailPath);

    SetDrawScreen(oldDrawScreen);
    DeleteGraph(thumbnailScreen);
    return saveResult == 0;
}

static bool SaveEditorKitThumbnailFromSelection(const char* kitName)
{
    // 2026-06-28: ワークスペース画面の切り抜きだとUIやカメラ状態でサムネがずれるため、選択範囲だけを専用画面に描いて保存する。
    if (!HasValidSelection())
        return false;

    int minX = min(selectStartX, selectEndX);
    int maxX = max(selectStartX, selectEndX);
    int minZ = min(selectStartZ, selectEndZ);
    int maxZ = max(selectStartZ, selectEndZ);

    const int thumbnailSize = 256;
    int thumbnailScreen = MakeScreen(thumbnailSize, thumbnailSize, TRUE);
    if (thumbnailScreen == -1)
        return false;

    int oldDrawScreen = GetDrawScreen();
    SetDrawScreen(thumbnailScreen);
    ClearDrawScreen();

    float centerX = (minX + maxX + 1) * BLOCK_SIZE * 0.5f;
    float centerZ = (minZ + maxZ + 1) * BLOCK_SIZE * 0.5f;
    float centerY = selectLayer * BLOCK_SIZE + BLOCK_SIZE * 0.25f;
    float rangeX = (maxX - minX + 1) * BLOCK_SIZE;
    float rangeZ = (maxZ - minZ + 1) * BLOCK_SIZE;
    // 2026-06-28: サムネごとに見え方がぶれないよう固定角度にしつつ、小さく写りすぎない距離へ寄せる。
    float cameraDist = max(900.0f, max(rangeX, rangeZ) * 1.18f);
    float cameraRotY = 0.78f;
    float cameraRotX = 0.58f;

    SetUseZBuffer3D(TRUE);
    SetWriteZBuffer3D(TRUE);
    SetCameraNearFar(10.0f, 50000.0f);
    SetCameraPositionAndTarget_UpVecY(
        VGet(
            centerX + sinf(cameraRotY) * cosf(cameraRotX) * cameraDist,
            centerY + 280.0f + sinf(cameraRotX) * cameraDist,
            centerZ + cosf(cameraRotY) * cosf(cameraRotX) * cameraDist),
        VGet(centerX, centerY, centerZ));

    for (int z = minZ; z <= maxZ; z++)
    {
        for (int x = minX; x <= maxX; x++)
        {
            DrawEditorKitThumbnailModel(FLOOR, FloorMap[selectLayer][z][x], x, selectLayer, z, FloorRot[selectLayer][z][x]);
            DrawEditorKitThumbnailModel(WALL, WallMapA[selectLayer][z][x], x, selectLayer, z, WallRotA[selectLayer][z][x]);
            DrawEditorKitThumbnailModel(WALL, WallMapB[selectLayer][z][x], x, selectLayer, z, WallRotB[selectLayer][z][x]);
            DrawEditorKitThumbnailModel(CORNER, CornerMap[selectLayer][z][x], x, selectLayer, z, CornerRot[selectLayer][z][x]);
            DrawEditorKitThumbnailModel(DECO, DecoMap[selectLayer][z][x], x, selectLayer, z, DecoRot[selectLayer][z][x]);
        }
    }

    char thumbPath[MAX_PATH];
    BuildEditorKitThumbnailPath(thumbPath, sizeof(thumbPath), kitName);
    bool saved = SaveDrawScreenReplaceFile(0, 0, thumbnailSize - 1, thumbnailSize - 1, thumbPath);

    SetDrawScreen(oldDrawScreen);
    DeleteGraph(thumbnailScreen);
    return saved;
}

static bool SaveEditorKitThumbnailFromCopyBuffer(const char* kitName)
{
    // 2026-06-28: ベース/自作どちらも、選択したキットからすぐサムネを作り直せるようコピー済みデータを描画する。
    if (!hasCopyData || copySizeX <= 0 || copySizeZ <= 0)
        return false;

    const int thumbnailSize = 256;
    int thumbnailScreen = MakeScreen(thumbnailSize, thumbnailSize, TRUE);
    if (thumbnailScreen == -1)
        return false;

    int oldDrawScreen = GetDrawScreen();
    SetDrawScreen(thumbnailScreen);
    ClearDrawScreen();

    float centerX = copySizeX * BLOCK_SIZE * 0.5f;
    float centerZ = copySizeZ * BLOCK_SIZE * 0.5f;
    float centerY = BLOCK_SIZE * 0.25f;
    float rangeX = copySizeX * BLOCK_SIZE;
    float rangeZ = copySizeZ * BLOCK_SIZE;
    float cameraDist = max(900.0f, max(rangeX, rangeZ) * 1.18f);
    float cameraRotY = 0.78f;
    float cameraRotX = 0.58f;

    SetUseZBuffer3D(TRUE);
    SetWriteZBuffer3D(TRUE);
    SetCameraNearFar(10.0f, 50000.0f);
    SetCameraPositionAndTarget_UpVecY(
        VGet(
            centerX + sinf(cameraRotY) * cosf(cameraRotX) * cameraDist,
            centerY + 280.0f + sinf(cameraRotX) * cameraDist,
            centerZ + cosf(cameraRotY) * cosf(cameraRotX) * cameraDist),
        VGet(centerX, centerY, centerZ));

    for (int z = 0; z < copySizeZ; z++)
    {
        for (int x = 0; x < copySizeX; x++)
        {
            DrawEditorKitThumbnailModel(FLOOR, CopyFloorMap[z][x], x, 0, z, CopyFloorRot[z][x]);
            DrawEditorKitThumbnailModel(WALL, CopyWallMapA[z][x], x, 0, z, CopyWallRotA[z][x]);
            DrawEditorKitThumbnailModel(WALL, CopyWallMapB[z][x], x, 0, z, CopyWallRotB[z][x]);
            DrawEditorKitThumbnailModel(CORNER, CopyCornerMap[z][x], x, 0, z, CopyCornerRot[z][x]);
            DrawEditorKitThumbnailModel(DECO, CopyDecoMap[z][x], x, 0, z, CopyDecoRot[z][x]);
        }
    }

    char thumbPath[MAX_PATH];
    BuildEditorKitThumbnailPath(thumbPath, sizeof(thumbPath), kitName);
    bool saved = SaveDrawScreenReplaceFile(0, 0, thumbnailSize - 1, thumbnailSize - 1, thumbPath);

    SetDrawScreen(oldDrawScreen);
    DeleteGraph(thumbnailScreen);
    return saved;
}

static void SavePendingEditorKitThumbnail()
{
    if (!pendingKitThumbnailSave)
        return;

    pendingKitThumbnailSave = false;

    if (SaveEditorKitThumbnailFromSelection(pendingKitThumbnailName))
        RefreshEditorKitList();
}

bool RefreshSelectedEditorKitThumbnail()
{
    // 2026-06-28: キットを選んだ後にボタン一つでサムネを再登録できるよう追加。
    if (selectedEditorKitIndex < 0 || selectedEditorKitIndex >= editorKitCount)
        return false;

    int targetIndex = selectedEditorKitIndex;
    if (editorKitThumbnailHandle[targetIndex] != -1)
    {
        DeleteGraph(editorKitThumbnailHandle[targetIndex]);
        editorKitThumbnailHandle[targetIndex] = -1;
    }

    if (!SaveEditorKitThumbnailFromCopyBuffer(editorKitNameList[targetIndex]))
        return false;

    ReloadEditorKitThumbnailHandle(targetIndex);
    return true;
}

static void WriteCurrentCopyBufferAsKit(const char* kitName)
{
    EnsureEditorKitDirectory();

    char path[MAX_PATH];
    BuildEditorKitPath(path, sizeof(path), kitName);
    ofstream ofs(path);
    if (!ofs.is_open())
        return;

    // 2026-06-25: 部屋/仕掛けのまとまりを再利用できるよう、コピー範囲と同じ情報をキットファイルへ保存する。
    ofs << "KIT," << kitName << "," << copySizeX << "," << copySizeZ << "\n";
    for (int z = 0; z < copySizeZ; z++)
    {
        for (int x = 0; x < copySizeX; x++)
        {
            ofs << "CELL," << z << "," << x << ","
                << CopyFloorMap[z][x] << "," << CopyFloorRot[z][x] << ","
                << CopyWallMapA[z][x] << "," << CopyWallMapB[z][x] << ","
                << CopyWallRotA[z][x] << "," << CopyWallRotB[z][x] << ","
                << CopyCornerMap[z][x] << "," << CopyCornerRot[z][x] << ","
                << CopyDecoMap[z][x] << "," << CopyDecoRot[z][x] << ","
                << CopyEventMap[z][x] << ","
                << CopyCollisionMap[z][x] << ","
                << CopyCollisionBoxOffsetXMap[z][x] << "," << CopyCollisionBoxOffsetZMap[z][x] << ","
                << CopyCollisionBoxSizeXMap[z][x] << "," << CopyCollisionBoxSizeZMap[z][x] << ","
                << CopyCollisionEdgeMap[z][x];

            for (int edge = 0; edge < 4; edge++)
                ofs << "," << CopyCollisionEdgeScaleMap[z][x][edge];
            for (int edge = 0; edge < 4; edge++)
                ofs << "," << CopyCollisionEdgeThicknessMap[z][x][edge];

            ofs << ","
                << CopyCollisionCornerScaleMap[z][x] << ","
                << CopyCollisionCornerThicknessMap[z][x] << ","
                << CopyCollisionCornerOffsetMap[z][x] << ","
                << CopyEventRot[z][x] << "\n";
        }
    }
}

static bool LoadEditorKitToCopyBuffer(const char* kitName)
{
    char path[MAX_PATH];
    BuildEditorKitPath(path, sizeof(path), kitName);
    ifstream ifs(path);
    if (!ifs.is_open())
    {
        ifs.open(GetEditorKitCsvPath());
        if (!ifs.is_open())
            return false;
    }

    ClearCopyBuffer();
    bool loadingTargetKit = false;

    string line;
    while (getline(ifs, line))
    {
        vector<string> cols = SplitCSV(line);
        if (cols.empty())
            continue;

        if (cols[0] == "KIT" && cols.size() >= 4)
        {
            loadingTargetKit = cols[1] == kitName;
            if (!loadingTargetKit)
                continue;

            copySizeX = max(1, min(COPY_MAX_X, stoi(cols[2])));
            copySizeZ = max(1, min(COPY_MAX_Z, stoi(cols[3])));
            hasCopyData = true;
        }
        else if (loadingTargetKit && cols[0] == "CELL" && cols.size() >= 31)
        {
            int z = stoi(cols[1]);
            int x = stoi(cols[2]);
            if (z < 0 || z >= COPY_MAX_Z || x < 0 || x >= COPY_MAX_X)
                continue;

            int c = 3;
            CopyFloorMap[z][x] = stoi(cols[c++]);
            CopyFloorRot[z][x] = stoi(cols[c++]);
            CopyWallMapA[z][x] = stoi(cols[c++]);
            CopyWallMapB[z][x] = stoi(cols[c++]);
            CopyWallRotA[z][x] = stoi(cols[c++]);
            CopyWallRotB[z][x] = stoi(cols[c++]);
            CopyCornerMap[z][x] = stoi(cols[c++]);
            CopyCornerRot[z][x] = stoi(cols[c++]);
            CopyDecoMap[z][x] = stoi(cols[c++]);
            CopyDecoRot[z][x] = stoi(cols[c++]);
            CopyEventMap[z][x] = stoi(cols[c++]);
            CopyEventRot[z][x] = 0;
            CopyCollisionMap[z][x] = stoi(cols[c++]);
            CopyCollisionBoxOffsetXMap[z][x] = stoi(cols[c++]);
            CopyCollisionBoxOffsetZMap[z][x] = stoi(cols[c++]);
            CopyCollisionBoxSizeXMap[z][x] = stoi(cols[c++]);
            CopyCollisionBoxSizeZMap[z][x] = stoi(cols[c++]);
            CopyCollisionEdgeMap[z][x] = stoi(cols[c++]);

            for (int edge = 0; edge < 4; edge++)
                CopyCollisionEdgeScaleMap[z][x][edge] = stoi(cols[c++]);
            for (int edge = 0; edge < 4; edge++)
                CopyCollisionEdgeThicknessMap[z][x][edge] = stoi(cols[c++]);

            CopyCollisionCornerScaleMap[z][x] = stoi(cols[c++]);
            CopyCollisionCornerThicknessMap[z][x] = stoi(cols[c++]);
            CopyCollisionCornerOffsetMap[z][x] = stoi(cols[c++]);
            // 2026-07-21: 古いキットCSVはEVENT回転列が無いため、列がある時だけ読む。
            CopyEventRot[z][x] = c < (int)cols.size() ? stoi(cols[c++]) : 0;
        }
    }

    return hasCopyData;
}

void SelectEditorKit(int index)
{
    if (index < 0 || index >= editorKitCount)
        return;

    selectedEditorKitIndex = index;
    if (LoadEditorKitToCopyBuffer(editorKitNameList[index]))
    {
        pasteMode = true;
        copyRotation = 0;
        selectMode = false;
        selecting = false;
    }
}

void RefreshMapNameList()
{
    // 2026-07-15: マップ一覧は直置きCSVではなく、maps/マップ名/map.csv を持つフォルダから作る。
    mapNameCount = 0;
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

            strcpy_s(mapNameList[mapNameCount], sizeof(mapNameList[mapNameCount]), findData.cFileName);
            mapNameCount++;
        } while (mapNameCount < MAP_NAME_LIST_MAX && FindNextFileA(handle, &findData));

        FindClose(handle);
    }

    if (mapNameCount <= 0)
    {
        strcpy_s(mapNameList[0], sizeof(mapNameList[0]), "map1");
        mapNameCount = 1;
    }

    if (selectedMapListIndex < 0)
        selectedMapListIndex = 0;
    if (selectedMapListIndex >= mapNameCount)
        selectedMapListIndex = mapNameCount - 1;

    strcpy_s(selectedMapName, sizeof(selectedMapName), mapNameList[selectedMapListIndex]);
}

static void SelectMapName(int direction)
{
    RefreshMapNameList();
    if (mapNameCount <= 0)
        return;

    selectedMapListIndex += direction;
    if (selectedMapListIndex < 0)
        selectedMapListIndex = mapNameCount - 1;
    if (selectedMapListIndex >= mapNameCount)
        selectedMapListIndex = 0;

    strcpy_s(selectedMapName, sizeof(selectedMapName), mapNameList[selectedMapListIndex]);
}

static void SyncSelectedMapNameToCurrent()
{
    if (gameCurrentMapName[0] == '\0')
        return;

    RefreshMapNameList();
    for (int i = 0; i < mapNameCount; i++)
    {
        if (strcmp(mapNameList[i], gameCurrentMapName) == 0)
        {
            selectedMapListIndex = i;
            strcpy_s(selectedMapName, sizeof(selectedMapName), gameCurrentMapName);
            return;
        }
    }

    strcpy_s(selectedMapName, sizeof(selectedMapName), gameCurrentMapName);
}
static void SyncSaveDescriptionFromKeyInput()
{
    if (saveDescriptionKeyInput != -1)
        GetKeyInputString(saveDescriptionInput, saveDescriptionKeyInput);
}

static void EnsureSaveDescriptionKeyInput()
{
    if (saveDescriptionKeyInput != -1)
        return;

    // 2026-07-22: 新規保存ボタンを押した瞬間にIME用KeyInputを作ると環境によって固まるため、DESC欄を選んだ時だけ遅延作成する。
    saveDescriptionKeyInput = MakeKeyInput(sizeof(saveDescriptionInput) - 1, FALSE, FALSE, FALSE);
    if (saveDescriptionKeyInput != -1)
        SetKeyInputString(saveDescriptionInput, saveDescriptionKeyInput);
}

static bool IsSaveDescriptionIMECandidateOpen()
{
    if (saveInputField != 1 || saveDescriptionKeyInput == -1 || GetActiveKeyInput() != saveDescriptionKeyInput)
        return false;

    const IMEINPUTDATA* imeData = GetIMEInputData();
    return imeData != nullptr && imeData->CandidateNum > 0;
}
static void SetSaveInputField(int field)
{
    // 2026-07-17: マップ説明欄だけDxLibのKeyInputを有効にし、日本語IME入力を受け取れるようにする。
    SyncSaveDescriptionFromKeyInput();
    saveInputField = field;

    if (saveInputField == 1)
    {
        EnsureSaveDescriptionKeyInput();
        if (saveDescriptionKeyInput != -1)
            SetActiveKeyInput(saveDescriptionKeyInput);
    }
    else
    {
        SetActiveKeyInput(-1);
    }
}

static void CloseSaveNameInput()
{
    // 2026-07-17: 保存/キャンセル時にKeyInputを破棄し、他の画面へ日本語入力状態を残さないようにする。
    SyncSaveDescriptionFromKeyInput();
    if (saveDescriptionKeyInput != -1)
    {
        DeleteKeyInput(saveDescriptionKeyInput);
        saveDescriptionKeyInput = -1;
    }
    SetActiveKeyInput(-1);
    saveNameInputActive = false;
}

static void StartSaveNameInput()
{
    // 2026-05-26: まだ保存名がない時だけ、F5で名前を付けて保存に入る。確定/取消もEnter/EscではなくF5/F9にして他入力と被らせない。
    strcpy_s(saveNameInput, sizeof(saveNameInput), gameCurrentMapName);
    strcpy_s(saveDescriptionInput, sizeof(saveDescriptionInput), gameCurrentMapDescription);

    if (saveDescriptionKeyInput != -1)
    {
        DeleteKeyInput(saveDescriptionKeyInput);
        saveDescriptionKeyInput = -1;
    }

    // 2026-07-22: 保存名だけ入力する時はKeyInputを作らず、DESC欄へ移動した時に日本語入力を準備する。
    saveNameInputActive = true;
    SetSaveInputField(0);
}
static void StartKitNameInput()
{
    // 2026-06-25: 選択範囲を名前付きキットとして登録できるよう、マップ保存とは別の名前入力を開始する。
    if (!HasValidSelection())
        return;

    strcpy_s(kitNameInput, sizeof(kitNameInput), "kit");
    kitNameInputActive = true;
}

void StartEditorKitRegistrationFromSelection()
{
    StartKitNameInput();
}

bool HasEditorRangeSelection()
{
    // 2026-07-21: UIボタン表示を、範囲選択中かつ有効範囲がある時だけコピー表示へ変えるため公開する。
    return selectMode && HasValidSelection();
}

bool StartEditorCopyFromSelectionButton()
{
    // 2026-07-21: 右パネルのコピークリックで、Ctrl+Cと同じくコピー後すぐ左クリック配置へ入れるようにする。
    if (!HasValidSelection())
    {
        selectMode = true;
        selecting = false;
        return false;
    }

    CopySelection();
    if (!hasCopyData)
        return false;

    pasteMode = true;
    copyRotation = 0;
    selectMode = false;
    selecting = false;
    return true;
}
// 2026-06-25: 上部バーの保存ボタンから、F5と同じ保存処理を呼べるよう追加。
bool IsEditorSaveInputActive()
{
    // 2026-07-17: 保存ダイアログ中はScene側のテストプレイ/タイトルUI入力を止めるため状態を公開する。
    return saveNameInputActive;
}
void SaveEditorFromUI()
{
    if (gameCurrentMapName[0] == '\0')
        StartSaveNameInput();
    else
    {
        StartSaveNameInput();
    }
}

void NewEditorMapFromUI()
{
    // 2026-07-15: 別名保存で前のマップを複製せず、空の新規マップとして作り始められるよう追加。
    ResetAllMap();
    currentMapIndex = 0;
    gameCurrentMapName[0] = '\0';
    gameCurrentMapDescription[0] = '\0';
    selectedMapName[0] = '\0';
    strcpy_s(saveNameInput, sizeof(saveNameInput), "map");
    saveDescriptionInput[0] = '\0';
    if (saveDescriptionKeyInput != -1)
    {
        DeleteKeyInput(saveDescriptionKeyInput);
        saveDescriptionKeyInput = -1;
    }
    // 2026-07-22: 新規保存直後のフリーズ回避のため、DESC欄を選ぶまでKeyInputは作らない。
    saveNameInputActive = true;
    SetSaveInputField(0);
    ClearEditorMapDirty();
}

static char GetTriggeredSaveInputChar(bool forDescription)
{
    bool shift = CheckHitKey(KEY_INPUT_LSHIFT) || CheckHitKey(KEY_INPUT_RSHIFT);

    struct KeyChar
    {
        int key;
        char lower;
        char upper;
    };

    static const KeyChar letters[] =
    {
        { KEY_INPUT_A, 'a', 'A' }, { KEY_INPUT_B, 'b', 'B' }, { KEY_INPUT_C, 'c', 'C' },
        { KEY_INPUT_D, 'd', 'D' }, { KEY_INPUT_E, 'e', 'E' }, { KEY_INPUT_F, 'f', 'F' },
        { KEY_INPUT_G, 'g', 'G' }, { KEY_INPUT_H, 'h', 'H' }, { KEY_INPUT_I, 'i', 'I' },
        { KEY_INPUT_J, 'j', 'J' }, { KEY_INPUT_K, 'k', 'K' }, { KEY_INPUT_L, 'l', 'L' },
        { KEY_INPUT_M, 'm', 'M' }, { KEY_INPUT_N, 'n', 'N' }, { KEY_INPUT_O, 'o', 'O' },
        { KEY_INPUT_P, 'p', 'P' }, { KEY_INPUT_Q, 'q', 'Q' }, { KEY_INPUT_R, 'r', 'R' },
        { KEY_INPUT_S, 's', 'S' }, { KEY_INPUT_T, 't', 'T' }, { KEY_INPUT_U, 'u', 'U' },
        { KEY_INPUT_V, 'v', 'V' }, { KEY_INPUT_W, 'w', 'W' }, { KEY_INPUT_X, 'x', 'X' },
        { KEY_INPUT_Y, 'y', 'Y' }, { KEY_INPUT_Z, 'z', 'Z' }
    };

    for (int i = 0; i < (int)(sizeof(letters) / sizeof(letters[0])); i++)
    {
        if (Input::IsKeyTrigger(letters[i].key))
            return shift ? letters[i].upper : letters[i].lower;
    }

    static const KeyChar numbers[] =
    {
        { KEY_INPUT_0, '0', '0' }, { KEY_INPUT_1, '1', '1' }, { KEY_INPUT_2, '2' , '2' },
        { KEY_INPUT_3, '3', '3' }, { KEY_INPUT_4, '4', '4' }, { KEY_INPUT_5, '5' , '5' },
        { KEY_INPUT_6, '6', '6' }, { KEY_INPUT_7, '7', '7' }, { KEY_INPUT_8, '8' , '8' },
        { KEY_INPUT_9, '9', '9' }
    };

    for (int i = 0; i < (int)(sizeof(numbers) / sizeof(numbers[0])); i++)
    {
        if (Input::IsKeyTrigger(numbers[i].key))
            return numbers[i].lower;
    }

    if (Input::IsKeyTrigger(KEY_INPUT_SPACE))
        return forDescription ? ' ' : '_';

    return '\0';
}

static char GetTriggeredSaveNameChar()
{
    return GetTriggeredSaveInputChar(false);
}

static void CommitSaveNameInput()
{
    if (saveNameInput[0] == '\0')
        return;

    // 2026-07-17: 確定時に名前と説明を現在マップ情報へ反映し、info.txtから後で読めるよう保存する。
    SyncSaveDescriptionFromKeyInput();
    strcpy_s(gameCurrentMapName, 64, saveNameInput);
    strcpy_s(gameCurrentMapDescription, sizeof(gameCurrentMapDescription), saveDescriptionInput);
    SaveMapAsCurrentName();
    SyncSelectedMapNameToCurrent();
    CloseSaveNameInput();
}

static void UpdateSaveNameInput()
{
    // 2026-07-22: 保存入力中に左上へ出していたACTIVEデバッグ文字は提出用に表示しない。
    int mx = 0;
    int my = 0;
    GetMousePoint(&mx, &my);
    int mouse = GetMouseInput();
    bool lTrigger = (mouse & MOUSE_INPUT_LEFT) && !(oldMouse & MOUSE_INPUT_LEFT);

    if (lTrigger)
    {
        if (mx >= 360 && mx <= 900 && my >= 294 && my <= 328)
        {
            SetSaveInputField(0);
            return;
        }
        if (mx >= 360 && mx <= 900 && my >= 338 && my <= 378)
        {
            SetSaveInputField(1);
            return;
        }
        if (mx >= 680 && mx <= 800 && my >= 404 && my <= 432)
        {
            CommitSaveNameInput();
            return;
        }
        if (mx >= 812 && mx <= 912 && my >= 404 && my <= 432)
        {
            CloseSaveNameInput();
            return;
        }
    }

    if (Input::IsKeyTrigger(KEY_INPUT_F9))
    {
        CloseSaveNameInput();
        return;
    }

    bool tabSwitchField = Input::IsKeyTrigger(KEY_INPUT_TAB) && (saveInputField == 0 || !IsSaveDescriptionIMECandidateOpen());
    if (tabSwitchField ||
        (saveInputField == 0 && (Input::IsKeyTrigger(KEY_INPUT_UP) || Input::IsKeyTrigger(KEY_INPUT_DOWN))))
    {
        // 2026-07-23: 説明欄でIME候補が出ている時だけTABは候補切替へ渡し、それ以外では入力欄切替に使う。
        SetSaveInputField(1 - saveInputField);
        return;
    }

    if (saveInputField == 0)
    {
        if (Input::IsKeyTrigger(KEY_INPUT_BACK))
        {
            size_t len = strlen(saveNameInput);
            if (len > 0)
                saveNameInput[len - 1] = '\0';
        }

        char c = GetTriggeredSaveInputChar(false);
        if (c != '\0')
        {
            size_t len = strlen(saveNameInput);
            if (len < sizeof(saveNameInput) - 1)
            {
                saveNameInput[len] = c;
                saveNameInput[len + 1] = '\0';
            }
        }
    }
    else
    {
        // 2026-07-17: 説明欄はKeyInputから毎フレーム取得し、IMEで確定した日本語を保存用バッファへ反映する。
        SyncSaveDescriptionFromKeyInput();
        if (saveDescriptionKeyInput != -1 && CheckKeyInput(saveDescriptionKeyInput) != 0)
        {
            // 2026-07-23: IME変換確定のEnterでDxLib側が入力完了扱いになっても、保存はF5なので説明欄を続けて編集できるよう戻す。
            SetActiveKeyInput(saveDescriptionKeyInput);
        }
    }
    
    if (Input::IsKeyTrigger(KEY_INPUT_F5))
        CommitSaveNameInput();

}
static void UpdateKitNameInput()
{
    if (Input::IsKeyTrigger(KEY_INPUT_F9))
    {
        kitNameInputActive = false;
        return;
    }

    if (Input::IsKeyTrigger(KEY_INPUT_BACK))
    {
        size_t len = strlen(kitNameInput);
        if (len > 0)
            kitNameInput[len - 1] = '\0';
    }

    char c = GetTriggeredSaveNameChar();
    if (c != '\0')
    {
        size_t len = strlen(kitNameInput);
        if (len < sizeof(kitNameInput) - 1)
        {
            kitNameInput[len] = c;
            kitNameInput[len + 1] = '\0';
        }
    }

    if (Input::IsKeyTrigger(KEY_INPUT_F5) && kitNameInput[0] != '\0')
    {
        SanitizeKitFileName(kitNameInput);
        CopySelection();
        if (hasCopyData)
        {
            WriteCurrentCopyBufferAsKit(kitNameInput);
            RequestEditorKitThumbnailSave(kitNameInput);
            RefreshEditorKitList();
        }
        kitNameInputActive = false;
    }
}

static void DrawSaveInputButton(int x1, int y1, int x2, int y2, const char* text, bool primary)
{
    int fill = primary ? GetColor(45, 120, 210) : GetColor(55, 60, 70);
    int border = primary ? GetColor(140, 205, 255) : GetColor(150, 160, 175);
    DrawBox(x1, y1, x2, y2, fill, TRUE);
    DrawBox(x1, y1, x2, y2, border, FALSE);
    DrawString(x1 + 28, y1 + 8, text, GetColor(255, 255, 255));
}

static void DrawSaveNameInput()
{
    if (!saveNameInputActive)
        return;

    // 2026-07-17: 保存時にマップ名と説明を同じダイアログで入力できるよう表示を拡張する。
    DrawBox(330, 236, 930, 450, GetColor(10, 10, 10), TRUE);
    DrawBox(330, 236, 930, 450, GetColor(255, 255, 255), FALSE);
    DrawString(360, 260, "マップ情報の保存", GetColor(255, 255, 0));

    int nameColor = saveInputField == 0 ? GetColor(255, 255, 120) : GetColor(220, 220, 220);
    int descColor = saveInputField == 1 ? GetColor(255, 255, 120) : GetColor(220, 220, 220);
    DrawBox(360, 294, 900, 328, saveInputField == 0 ? GetColor(42, 48, 58) : GetColor(22, 25, 32), TRUE);
    DrawBox(360, 294, 900, 328, saveInputField == 0 ? GetColor(255, 255, 120) : GetColor(90, 95, 105), FALSE);
    DrawString(378, 303, saveInputField == 0 ? "> マップ名" : "  マップ名", nameColor);
    DrawFormatString(506, 303, GetColor(255, 255, 255), "%s.csv", saveNameInput[0] != '\0' ? saveNameInput : "_");

    DrawBox(360, 338, 900, 378, saveInputField == 1 ? GetColor(42, 48, 58) : GetColor(22, 25, 32), TRUE);
    DrawBox(360, 338, 900, 378, saveInputField == 1 ? GetColor(255, 255, 120) : GetColor(90, 95, 105), FALSE);
    DrawString(378, 350, saveInputField == 1 ? "> 説明" : "  説明", descColor);
    if (saveInputField == 1 && saveDescriptionKeyInput != -1)
    {
        // 2026-07-17: KeyInputの描画を使い、IME変換中の日本語候補も説明欄に表示する。
        DrawKeyInputString(506, 350, saveDescriptionKeyInput, TRUE);
    }
    else
    {
        DrawFormatString(506, 350, GetColor(255, 255, 255), "%s", saveDescriptionInput[0] != '\0' ? saveDescriptionInput : "_");
    }

    DrawSaveInputButton(680, 404, 800, 432, "保存", true);
    DrawSaveInputButton(812, 404, 912, 432, "戻る", false);
    DrawString(360, 386, "TAB/クリック: 入力欄   F5: 保存   BACKSPACE: 削除   F9: 戻る", GetColor(180, 220, 255));
}

static void DrawKitNameInput()
{
    if (!kitNameInputActive)
        return;

    DrawBox(320, 270, 960, 410, GetColor(10, 10, 10), TRUE);
    DrawBox(320, 270, 960, 410, GetColor(255, 255, 255), FALSE);
    DrawString(350, 292, "KIT NAME", GetColor(255, 220, 80));
    DrawFormatString(350, 326, GetColor(255, 255, 255), "%s.kit", kitNameInput[0] != '\0' ? kitNameInput : "_");
    DrawString(350, 356, "F5: KIT登録   BACKSPACE: DELETE   F9: CANCEL", GetColor(180, 220, 255));
    DrawString(350, 382, "登録元: 現在の範囲選択", GetColor(210, 230, 255));
}
#pragma region ===== 初期化 =====

void InitEditor()
{
    ResetAllMap();
    ClearCopyBuffer();

    for (int i = 0; i < MODEL_MAX; i++)
    {
        modelHandles[i] = -1;
        paletteTex[i] = -1;
        modelOffsetX[i] = 0.0f;
        modelOffsetZ[i] = 0.0f;
        modelOffsetY[i] = 0.0f;
    }

    camRotY = 0.785398f;
    camRotX = 0.45f;
    camDist = 6500.0f;
    // 2026-07-21: エディター開始時はマップ中央を見る状態から始める。
    camTargetX = BLOCK_NUM_X * BLOCK_SIZE * 0.5f;
    camTargetZ = BLOCK_NUM_Z * BLOCK_SIZE * 0.5f;

    LoadEditorModelConfig();
    RefreshMapNameList();
    RefreshEditorKitList();

    InitEditorCollisionTable();
    RefreshMapNameList();

    // 2026-
    // : エディターを開いた瞬間に、タイトルで選んだCSVまたは一覧の先頭CSVを読み込む。
    if (gameCurrentMapName[0] != '\0')
    {
        LoadEditorMapNow(gameCurrentMapName);
        SyncSelectedMapNameToCurrent();
    }
    else if (selectedMapName[0] != '\0')
    {
        LoadEditorMapNow(selectedMapName);
    }

    ClearEditorMapDirty();
}

#pragma endregion


#pragma region ===== 更新 =====

void UpdateEditor()
{

    int mx = 0;
    int my = 0;
    GetMousePoint(&mx, &my);

    int mouse = GetMouseInput();
    int lClick = mouse & MOUSE_INPUT_LEFT;
    int rClick = mouse & MOUSE_INPUT_RIGHT;

    bool lTrigger = lClick && !(oldMouse & MOUSE_INPUT_LEFT);
    bool rawRTrigger = rClick && !(oldMouse & MOUSE_INPUT_RIGHT);
    bool rawRRelease = !rClick && (oldMouse & MOUSE_INPUT_RIGHT);
    bool rTrigger = false;

    bool ctrl =
        CheckHitKey(KEY_INPUT_LCONTROL) ||
        CheckHitKey(KEY_INPUT_RCONTROL);

    bool shift =
        CheckHitKey(KEY_INPUT_LSHIFT) ||
        CheckHitKey(KEY_INPUT_RSHIFT);

    // 2026-07-21: 未選択時だけ左ドラッグをカメラ移動にするため、現在タブで有効な素材選択かを先に判定する。
    bool hasModelSelected = GetSelectedModel() >= 0;

    // 2026-06-25: 新しい上下左右パネルを除いた場所だけを3Dワークスペースとして扱う。
    // 折りたたんだ右パネル領域もレイキャスト可能になる。
    bool isUI = !IsEditorWorkspacePoint(mx, my);

    if (loadMapConfirmActive)
    {
        UpdateLoadMapConfirm();
        oldMX = mx;
        oldMY = my;
        oldMouse = mouse;
        oldClick = lClick;
        return;
    }
    
    
    //05-29-マップ名前保存可能のため追加

    if (kitNameInputActive)
    {
        UpdateKitNameInput();

        oldMX = mx;
        oldMY = my;
        oldMouse = mouse;
        oldClick = lClick;
        return;
    }

    if (!saveNameInputActive && Input::IsKeyTrigger(KEY_INPUT_F5))
    {
        StartSaveNameInput();

        oldMX = mx;
        oldMY = my;
        oldMouse = mouse;
        oldClick = lClick;
        return;
    }

    if (saveNameInputActive)
    {
        UpdateSaveNameInput();

        oldMX = mx;
        oldMY = my;
        oldMouse = mouse;
        oldClick = lClick;
        return;
    }

#pragma region カメラ操作

    static bool rightCameraDragging = false;
    static int rightMouseStartX = 0;
    static int rightMouseStartY = 0;
    static int rightMouseHoldFrame = 0;

    if (rawRTrigger)
    {
        // 2026-07-22: 右クリックは短押し削除と長押しカメラ回転を分けるため、押し始め位置と時間を記録する。
        rightCameraDragging = false;
        rightMouseStartX = mx;
        rightMouseStartY = my;
        rightMouseHoldFrame = 0;
    }

    if (rClick)
    {
        rightMouseHoldFrame++;
        int dragX = mx - rightMouseStartX;
        int dragY = my - rightMouseStartY;
        if (!isUI && !ctrl && (rightCameraDragging || dragX * dragX + dragY * dragY >= 36))
        {
            // 2026-07-22: Shift+左ドラッグをやめ、右クリック長押し/ドラッグで視点回転できるようにする。
            rightCameraDragging = true;
            camRotY += (mx - oldMX) * 0.01f;
            camRotX += (my - oldMY) * 0.01f;

            if (camRotX > 1.2f) camRotX = 1.2f;
            if (camRotX < -1.2f) camRotX = -1.2f;
        }
    }

    if (rawRRelease)
    {
        // 2026-07-22: カメラ操作にならなかった短い右クリックだけを、従来の削除/キャンセル入力として扱う。
        rTrigger = !rightCameraDragging;
        rightCameraDragging = false;
        rightMouseHoldFrame = 0;
    }

    if (!hasModelSelected && !isUI && !ctrl && !pasteMode && !collisionEditMode && !selectMode && !eraserMode && lClick)
    {
        // 2026-07-21: 何も選んでいない時の左ドラッグは、見ている方向基準でエディターカメラを平行移動する。
        float dragX = (float)(mx - oldMX);
        float panSpeed = max(4.0f, camDist * 0.0015f);
        float rightX = cosf(camRotY);
        float rightZ = -sinf(camRotY);

        camTargetX += rightX * dragX * panSpeed;
        camTargetZ += rightZ * dragX * panSpeed;
    }
    oldMX = mx;
    oldMY = my;

    int wheel = GetMouseWheelRotVol();
    bool wheelUsedByUI = isUI && UpdateEditorUIWheel(mx, my, wheel);
    // 2026-06-15: 右側素材欄のスクロール中は、同じホイール入力でカメラ距離を変えない。
    if (!wheelUsedByUI && !(collisionEditMode && collisionEdgeEditMode))
    {
        // 2026-07-21: ホイール下方向で十分に引けるよう、距離に応じたズーム量と広めの上限に調整する。
        float zoomStep = max(300.0f, camDist * 0.08f);
        camDist -= wheel * zoomStep;
    }
    camDist = max(800.0f, min(24000.0f, camDist));

#pragma endregion


#pragma region キー操作

    // 2026-06-24: 当たり判定編集画面から確実に戻れるよう、Escでも編集を終了する。
    // 以前は編集状態だけOFFになっても表示フラグが残り、戻れていないように見えていた。
    if (editorScreenMode == 1 && Input::IsKeyTrigger(KEY_INPUT_ESCAPE))
    {
        editorScreenMode = 0;
        collisionEditMode = false;
        showCollisionDebug = false;
        collisionBoxDragging = false;
    }
    else if (!pasteMode && multiSelectCount > 0 && Input::IsKeyTrigger(KEY_INPUT_ESCAPE))
    {
        // 2026-06-25: 離れた複数選択をEscで解除できるよう追加。
        ClearMultiSelection();
    }
    else if (!pasteMode && Input::IsKeyTrigger(KEY_INPUT_ESCAPE) && selectedModel >= 0)
    {
        // 2026-07-21: Escで素材選択を外し、左ドラッグのカメラ移動へすぐ切り替えられるようにする。
        selectedModel = -1;
    }

    if (ctrl && Input::IsKeyTrigger(KEY_INPUT_Z))
    {
        UndoMap();
        selecting = false;
        lastBrushX = -1;
        lastBrushZ = -1;
        lastBrushLayer = -1;
    }

    if (Input::IsKeyTrigger(KEY_INPUT_Q))
        currentLayer = max(0, currentLayer - 1);

    if (Input::IsKeyTrigger(KEY_INPUT_E))
        currentLayer = min(BLOCK_NUM_Y - 1, currentLayer + 1);

    if (Input::IsKeyTrigger(KEY_INPUT_R))
    {
        // 2026-06-24:
        if (pasteMode)
            copyRotation = (copyRotation + 1) % 4;
        else if (shift && multiSelectCount > 0)
            RotateMultiSelectedCells();
        else if (shift && selectMode && HasValidSelection())
            RotateRangeSelectedCells();
        else
            currentRot = (currentRot + 1) % 4;
    }

    if (Input::IsKeyTrigger(KEY_INPUT_B))
        brushMode = !brushMode;

    // 2026-07-08: ENEMYタブでPを押すと、敵配置ではなく巡回ポイント編集に切り替える。
    if (currentTab == ENEMY && Input::IsKeyTrigger(KEY_INPUT_P))
        enemyPatrolEditMode = !enemyPatrolEditMode;

    if (!ctrl && Input::IsKeyTrigger(KEY_INPUT_V))
    {
        selectMode = !selectMode;
        selecting = false;
    }

    if (!ctrl && Input::IsKeyTrigger(KEY_INPUT_C))
    {
        PushUndo();
        ClearCurrentLayer();
    }

    // 2026-07-21: 矢印キー単体でマップ切替確認に入ると、配置操作が止まったように見えるためCtrl同時押しに限定する。
    if (ctrl && Input::IsKeyTrigger(KEY_INPUT_RIGHT))
    {
        if (mapNameCount > 0)
        {
            int nextMapListIndex = selectedMapListIndex + 1;

            if (nextMapListIndex >= mapNameCount)
            {
                nextMapListIndex = 0;
            }

            RequestEditorLoadMapFromList(nextMapListIndex);
        }
    }
    // 2026-07-21: 左方向のマップ切替も右方向と同じくCtrl同時押しにして、通常編集操作を邪魔しない。
    if (ctrl && Input::IsKeyTrigger(KEY_INPUT_LEFT))
    {
        if (mapNameCount > 0)
        {
            int nextMapListIndex = selectedMapListIndex - 1;

            if (nextMapListIndex < 0)
            {
                nextMapListIndex = mapNameCount - 1;
            }

            RequestEditorLoadMapFromList(nextMapListIndex);
        }
    }

    if (Input::IsKeyTrigger(KEY_INPUT_F5))
    {
        // 2026-05-26: 初回は名前を付けて保存、保存名がある時は同じCSVへ上書きする。
        if (gameCurrentMapName[0] == '\0')
            StartSaveNameInput();
        else
        {
            SaveMap();
            SyncSelectedMapNameToCurrent();
        }
    }

    if (Input::IsKeyTrigger(KEY_INPUT_F9))
    {
        if (gameCurrentMapName[0] != '\0')
            RequestEditorLoadMap(gameCurrentMapName);
        else
            RequestEditorLoadMap(currentMapIndex);
    }

#pragma endregion


#pragma region UIクリック

    // 2026-07-22: 色変更UIは長押し/ドラッグ調整、通常ボタンは押した瞬間だけ処理できるようUIへ押下状態を渡す。
    if (isUI || (oldMouse & MOUSE_INPUT_LEFT))
        UpdateEditorUI(mx, my, lClick);

#pragma endregion


#pragma region レイキャスト

    hoverX = -1;
    hoverZ = -1;
    // 2026-05-20: BoxCollider風編集のドラッグに使うため、毎フレームのマウスワールド座標を初期化する。
    hoverWorldX = 0.0f;
    hoverWorldZ = 0.0f;

    if (!isUI)
    {
        VECTOR nearPos = ConvScreenPosToWorldPos(VGet((float)mx, (float)my, 0.0f));
        VECTOR farPos = ConvScreenPosToWorldPos(VGet((float)mx, (float)my, 1.0f));

        float targetY = currentLayer * BLOCK_SIZE;
        float dy = farPos.y - nearPos.y;

        if (fabsf(dy) > 0.0001f)
        {
            float t = (targetY - nearPos.y) / dy;

            if (t >= 0.0f && t <= 1.0f)
            {
                VECTOR hit = VGet(0.0f, targetY, 0.0f);

                hit.x = nearPos.x + (farPos.x - nearPos.x) * t;
                hit.z = nearPos.z + (farPos.z - nearPos.z) * t;

                hoverWorldX = hit.x;
                hoverWorldZ = hit.z;
                hoverX = WorldToCell(hit.x);
                hoverZ = WorldToCell(hit.z);
            }
        }
    }

#pragma endregion

    int layer = currentLayer;
    int x = hoverX;
    int z = hoverZ;
    bool canAccess = IsMapPosValid(layer, z, x);
    bool isEnemyPatrolEditing = currentTab == ENEMY && enemyPatrolEditMode;
    // 2026-07-22: 巡回編集ONのままでも、まだ対象敵を選んでいない時は敵本体を配置できるようにする。
    bool canPlaceEnemyBeforePatrolTarget = isEnemyPatrolEditing && !IsSelectedPatrolEnemyValid();

    
    // 2026-07-22: 段差6は隣の上層床へ自動接続するため、Lキーでの手動ON/OFF処理は使わない。
if (!pasteMode && !collisionEditMode && !isUI && canAccess && ctrl && lTrigger)
    {
        // 2026-06-29: Shiftを視点回転専用に残すため、複数選択はShift+クリックからCtrl+クリックへ移す。
        ToggleMultiSelectionCell(layer, z, x);
        selecting = false;
        lTrigger = false;
    }

    if (!pasteMode && !collisionEditMode && multiSelectCount > 0 && rTrigger)
    {
        // 2026-06-25: 複数選択中の右クリックは削除ではなく選択解除にする。
        ClearMultiSelection();
        rTrigger = false;
    }

    // 2026-07-08: 巡回編集中は、敵クリックで対象選択、空きマス左クリックでポイント追加、右クリックでポイント削除。
    if (!pasteMode && !collisionEditMode && !selectMode && !shift && !isUI &&
        isEnemyPatrolEditing && canAccess)
    {
        if (lTrigger)
        {
            if (EnemyMap[layer][z][x] >= 0)
            {
                SelectPatrolEnemy(layer, z, x);
                // 2026-07-22: 敵を巡回対象として選んだクリックが、そのまま敵配置へ流れないようにする。
                lTrigger = false;
            }
            else if (IsSelectedPatrolEnemyValid())
            {
                PushUndo();
                AddPatrolPointToSelectedEnemy(z, x);
                lTrigger = false;
            }
            else
            {
                // 2026-07-22: 対象敵が未選択の時はクリックを消費せず、下の敵配置処理へ渡す。
            }
        }
        else if (rTrigger)
        {
            if (IsSelectedPatrolEnemyValid())
            {
                PushUndo();
                RemovePatrolPointFromSelectedEnemy(z, x);
            }
            rTrigger = false;
        }
    }


#pragma region コピー / 貼り付け

    if (ctrl && Input::IsKeyTrigger(KEY_INPUT_C))
    {
        CopySelection();

        // 2026-06-24: コピー後すぐ配置できるよう、Ctrl+C成功時にプレビューモードへ入る。
        if (hasCopyData)
        {
            pasteMode = true;
            copyRotation = 0;

            // 2026-06-24: コピー元の固定された青枠と、移動する貼り付け枠が
            // 同時表示されないよう、コピー成功後は範囲選択モードを終了する。
            selectMode = false;
            selecting = false;
        }
    }

    // 2026-06-24: Ctrl+Vは即時確定ではなく、位置を確認できるプレビューモードを開始する。
    if (ctrl && Input::IsKeyTrigger(KEY_INPUT_V) && hasCopyData)
    {
        pasteMode = true;
        copyRotation = 0;
    }

    if (pasteMode)
    {
        if (Input::IsKeyTrigger(KEY_INPUT_ESCAPE) || rTrigger)
        {
            // 2026-06-24: Esc/右クリックはマップを変更せずキャンセルする。
      
            pasteMode = false;
            rTrigger = false;
        }
        else if (!isUI && canAccess && lTrigger)
        {
            // 2026-06-24: 1回の配置を1つのUndo単位にする。配置後も連続スタンプできる。
            PushUndo();
            PasteSelectionRotated(layer, x, z, copyRotation);
            // 2026-07-22: コピー配置を1回置いたら、元の範囲選択の青枠を残さない。
            ClearRangeSelectionRect();
        }
    }

#pragma endregion


#pragma region 範囲選択

    if (selectMode && !pasteMode && !shift && !isUI && canAccess)
    {
        // 2026-07-21: 範囲選択は左クリックドラッグで行い、コピー後の配置も左クリックで確定できる流れに戻す。
        if (lTrigger)
        {
            selecting = true;
            selectLayer = layer;
            selectStartX = x;
            selectStartZ = z;
            selectEndX = x;
            selectEndZ = z;
        }

        if (selecting && lClick)
        {
            selectEndX = x;
            selectEndZ = z;
        }

        if (selecting && !lClick)
            selecting = false;

    }

#pragma endregion


    // 2026-06-02: コライダー専用画面では常に当たり判定表示を有効にし、配置操作と分けて触れるよう追加。
    if (editorScreenMode == 1)
    {
        // 2026-06-24: 当たり判定編集へ切り替えた時はコピー配置状態を終了し、
        // 編集操作と貼り付け操作が同時に有効になることを防ぐ。
        pasteMode = false;
        selectMode = false;
        selecting = false;
        collisionEditMode = true;
        showCollisionDebug = true;
    }
    else if (!collisionEditMode)
    {
        // 2026-06-24: 編集終了後に表示だけ残ってモデルが透け続けないようにする。
        showCollisionDebug = false;
    }

#pragma region 当たり判定伸縮編集

    // 2026-05-13: ホイール編集を長さモード/厚みモードで切り替え、範囲外セルを触らないよう修正。
    if (collisionEditMode && collisionEdgeEditMode && !selectMode && !isUI && canAccess && wheel != 0)
    {
        if (CornerMap[layer][z][x] >= 0)
        {
            if (collisionDepthEditMode)
            {
                int thick = CollisionCornerThicknessMap[layer][z][x] + wheel * 10;
                thick = max(20, min(500, thick));
                CollisionCornerThicknessMap[layer][z][x] = thick;
            }
            else
            {
                int scale = CollisionCornerScaleMap[layer][z][x] + wheel * 10;
                scale = max(20, min(200, scale));
                CollisionCornerScaleMap[layer][z][x] = scale;
            }
        }
        else
        {
            int mask = CollisionEdgeMap[layer][z][x] >= 0 ? CollisionEdgeMap[layer][z][x] : 0;
            CollisionEdgeMap[layer][z][x] = mask | GetCollisionEdgeBit(currentRot);

            if (collisionDepthEditMode)
            {
                int thick = CollisionEdgeThicknessMap[layer][z][x][currentRot] + wheel * 10;
                thick = max(20, min(300, thick));
                CollisionEdgeThicknessMap[layer][z][x][currentRot] = thick;
            }
            else
            {
                int scale = CollisionEdgeScaleMap[layer][z][x][currentRot] + wheel * 10;
                scale = max(20, min(200, scale));
                CollisionEdgeScaleMap[layer][z][x][currentRot] = scale;
            }
        }
    }
#pragma endregion

#pragma region 当たり判定編集

    // 2026-05-20: セル当たり判定は辺/厚み数値ではなく、BoxCollider風の箱を直接ドラッグして編集できるよう変更。
    if (collisionEditMode && !selectMode && !shift && !isUI && canAccess)
    {
        if (lTrigger)
        {
            PushUndo();

            if (collisionEdgeEditMode)
            {
                int mask = CollisionEdgeMap[layer][z][x] >= 0 ? CollisionEdgeMap[layer][z][x] : 0;
                CollisionEdgeMap[layer][z][x] = mask | GetCollisionEdgeBit(currentRot);
                CollisionEdgeScaleMap[layer][z][x][currentRot] = 100;
                CollisionEdgeThicknessMap[layer][z][x][currentRot] = 100;
            }
            else
            {
                if (CollisionMap[layer][z][x] < 0)
                {
                    CollisionMap[layer][z][x] = 1;
                    ResetCollisionBox(layer, z, x);
                }

                collisionBoxDragMode = GetCollisionBoxDragMode(layer, z, x, hoverWorldX, hoverWorldZ);
                if (collisionBoxDragMode == 0)
                    collisionBoxDragMode = 5;

                collisionBoxDragging = true;
                collisionBoxDragLayer = layer;
                collisionBoxDragX = x;
                collisionBoxDragZ = z;
                // 2026-05-20: 配置/クリックしたBOXを右側パネルの編集対象として選択する。
                selectedCollisionLayer = layer;
                selectedCollisionX = x;
                selectedCollisionZ = z;
                ApplyCollisionBoxDrag(layer, z, x, collisionBoxDragMode, hoverWorldX, hoverWorldZ);
            }
        }
        else if (rTrigger)
        {
            PushUndo();

            if (collisionEdgeEditMode)
            {
                int mask = CollisionEdgeMap[layer][z][x] >= 0 ? CollisionEdgeMap[layer][z][x] : 0;
                mask &= ~GetCollisionEdgeBit(currentRot);
                CollisionEdgeMap[layer][z][x] = mask > 0 ? mask : -1;
            }
            else
            {
                CollisionMap[layer][z][x] = -1;
                ResetCollisionBox(layer, z, x);
                if (selectedCollisionLayer == layer && selectedCollisionX == x && selectedCollisionZ == z)
                {
                    selectedCollisionLayer = -1;
                    selectedCollisionX = -1;
                    selectedCollisionZ = -1;
                }
                collisionBoxDragging = false;
            }
        }
    }

    if (collisionBoxDragging && lClick && !collisionEdgeEditMode)
    {
        ApplyCollisionBoxDrag(collisionBoxDragLayer, collisionBoxDragZ, collisionBoxDragX, collisionBoxDragMode, hoverWorldX, hoverWorldZ);
    }

    if (!lClick)
    {
        collisionBoxDragging = false;
        collisionBoxDragMode = 0;
    }

#pragma endregion
#pragma region 配置

    bool brushPlace =
        brushMode &&
        lClick &&
        canAccess &&
        (x != lastBrushX || z != lastBrushZ || layer != lastBrushLayer);

    bool singlePlace =
        !brushMode &&
        lTrigger &&
        canAccess;

    if (hasModelSelected && !eraserMode && !pasteMode && !collisionEditMode && !selectMode && !shift && !isUI && (!isEnemyPatrolEditing || canPlaceEnemyBeforePatrolTarget) && (singlePlace || brushPlace))
    {
        PushUndo();

        int model = GetSelectedModel();
        if (IsEditorPlacementDisabled(currentTab, model))
            return;

        if (currentTab == FLOOR)
        {
            FloorMap[layer][z][x] = model;
            FloorRot[layer][z][x] = currentRot;
            // 2026-07-21: 配置時点の色変更ボタンの色を床へ保存する。
            FloorColorMap[layer][z][x] = editorColorIndex;
        }
        else if (currentTab == WALL)
        {
            int wallRot = GetEditorPlacementRot(WALL, z, x);
            if (WallMapA[layer][z][x] >= 0 && WallRotA[layer][z][x] == wallRot)
            {
                // 2026-06-28: 同じセルの同じ辺に壁を重ねると位置がずれて見えるため、追加ではなく置き換える。
                WallMapA[layer][z][x] = model;
                WallColorMapA[layer][z][x] = editorColorIndex;
            }
            else if (WallMapB[layer][z][x] >= 0 && WallRotB[layer][z][x] == wallRot)
            {
                // 2026-06-28: 壁B側も同じ辺は二重配置せず、見た目とミニマップの重なりを防ぐ。
                WallMapB[layer][z][x] = model;
                WallColorMapB[layer][z][x] = editorColorIndex;
            }
            else if (WallMapA[layer][z][x] == -1)
            {
                WallMapA[layer][z][x] = model;
                WallRotA[layer][z][x] = wallRot;
                WallColorMapA[layer][z][x] = editorColorIndex;
            }
            else if (WallMapB[layer][z][x] == -1)
            {
                WallMapB[layer][z][x] = model;
                WallRotB[layer][z][x] = wallRot;
                WallColorMapB[layer][z][x] = editorColorIndex;
            }
            else
            {
                WallMapB[layer][z][x] = model;
                WallRotB[layer][z][x] = wallRot;
                WallColorMapB[layer][z][x] = editorColorIndex;
            }
        }
        else if (currentTab == CORNER)
        {
            CornerMap[layer][z][x] = model;
            CornerRot[layer][z][x] = currentRot;
            CornerColorMap[layer][z][x] = editorColorIndex;
            // 2026-07-23: 角だけ置くと床が抜けて見えるため、床が空なら標準床も同時に置く。
            if (FloorMap[layer][z][x] < 0)
            {
                FloorMap[layer][z][x] = 0;
                FloorRot[layer][z][x] = 0;
                FloorColorMap[layer][z][x] = 0;
            }
            // 2026-05-13: コーナー配置時に当たり判定調整値を標準値へ戻すため追加。
            CollisionCornerScaleMap[layer][z][x] = 100;
            CollisionCornerThicknessMap[layer][z][x] = 100;
            CollisionCornerOffsetMap[layer][z][x] = 0;
        }
        else if (currentTab == DECO)
        {
            DecoMap[layer][z][x] = model;
            DecoRot[layer][z][x] = currentRot;
            DecoColorMap[layer][z][x] = editorColorIndex;
        }
        else if (currentTab == ENEMY)
        {
            // 2026-07-23: 敵は足場がないセルには置けないようにする。
            if (IsEditorEnemyModelId(model) && FloorMap[layer][z][x] >= 0)
            {
                EnemyMap[layer][z][x] = GetEditorEnemyIdFromModel(model);
                ClearEnemyPatrolPoints(layer, z, x);
                SelectPatrolEnemy(layer, z, x);
            }
        }
        else if (currentTab == EVENT)
        {
            PlaceEvent(layer, z, x, GetEditorEventIdFromModel(model));
        }

        lastBrushX = x;
        lastBrushZ = z;
        lastBrushLayer = layer;
    }

    if (!lClick)
    {
        lastBrushX = -1;
        lastBrushZ = -1;
        lastBrushLayer = -1;
    }

#pragma endregion


#pragma region 削除

    bool eraserClick = eraserMode && (singlePlace || brushPlace);
    if (!pasteMode && !collisionEditMode && !selectMode && !shift && !isUI && !isEnemyPatrolEditing && (rTrigger || eraserClick) && canAccess)
    {
        bool changed = false;

        if (eraserMode)
        {
            if (eraserTarget == ERASER_TARGET_ALL)
            {
                if (HasEditorCellContent(layer, z, x))
                {
                    changed = TryClearEditorCellAllKinds(layer, z, x);
                }
            }
            else
            {
                changed = TryClearEditorCellByTarget(layer, z, x, eraserTarget);
            }
        }
        else
        {
            int target = GetEraserTargetFromTab(currentTab);

            if (currentTab == ENEMY)
            {
                if (EnemyMap[layer][z][x] == GetEditorEnemyIdFromModel(GetSelectedModel()))
                {
                    changed = TryClearEditorCellByTarget(layer, z, x, ERASER_TARGET_ENEMY);
                }
            }
            else if (target >= 0)
            {
                changed = TryClearEditorCellByTarget(layer, z, x, target);
            }
        }

        if (changed)
        {
            MarkEditorMapDirty();
        }
    }

#pragma endregion

    oldMouse = mouse;
    oldClick = lClick;
}

#pragma endregion


#pragma region ===== 描画 =====

#include "MapEditorRendering.inl"
