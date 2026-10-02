#pragma once
#include "DxLib.h"

// 2026-06-02: エディターのEVENTタブからゲーム用ギミックを置けるよう、イベントIDを名前で扱うため追加。
enum GameEventId
{
    GAME_EVENT_START = 0,
    GAME_EVENT_GOAL = 1,
    GAME_EVENT_PICKUP = 2,
    GAME_EVENT_HAZARD = 3,
    GAME_EVENT_KEY = 4,
    GAME_EVENT_LOCKED_DOOR = 5,
    // 2026-06-15: 一度通った地点へ復帰できる、任意チェックポイント用として追加。
    GAME_EVENT_CHECKPOINT = 7
};

// 2026-06-02: ゲーム中の「取るもの/避けるもの/鍵/扉」をSceneからまとめて扱うため追加。
void InitGameObjects();
void UpdateGameObjects(VECTOR playerPos, int playerLayer, bool& playerDamaged);
void DrawGameObjects();
void DrawGameObjectHUD();
// 2026-07-21: 鍵扉が開くまでプレイヤー移動を止めるため、マップ衝突側から問い合わせる。
bool HitActiveLockedDoorObject(int layer, float worldX, float worldZ, float radius);

// 2026-06-15: ライフが残っている時の復帰地点を、STARTまたはチェックポイントとして扱うため追加。
VECTOR GetGameRespawnPosition();
void ResetGameCheckpoint();

// 2026-06-02: ゴール判定とHUD表示で鍵・収集状況を確認するため追加。
bool IsGameObjectGoalUnlocked();
int GetGameObjectKeyCount();
int GetGameObjectPickupCount();
int GetGameObjectPickupTotal();

