#pragma once
#include "DxLib.h"

// 2026-06-12: 敵をGameEventIdから完全に分離するため、敵専用IDを追加。
enum GameEnemyId
{
    GAME_ENEMY_THROW = 0
};

// 2026-06-12: 敵の初期化・更新・描画をGameObjectsから分離して扱うため追加。
void InitGameEnemies();
void UpdateGameEnemies(VECTOR playerPos, int playerLayer, bool& playerDamaged);
void DrawGameEnemies();

// 2026-06-12: CSV以外の処理から任意のタイミングで敵を生成できるよう追加。
void SpawnGameEnemy(int enemyId, int layer, int x, int z);
// 2026-06-12: 生成に成功したか呼び出し元で判断できるよう追加。
bool TrySpawnGameEnemy(int enemyId, int layer, int x, int z);
// 2026-06-12: 同じセルへの重複生成を避けられるよう追加。
bool HasGameEnemyAt(int layer, int x, int z);
// 2026-06-12: ワールド座標しか持っていない処理から敵を生成しやすくするため追加。
bool TrySpawnGameEnemyAtWorld(int enemyId, VECTOR pos);

// 2026-06-12: 他の処理から敵のワールド座標を取りやすくするため追加。
VECTOR GetGameEnemyWorldPosition(int layer, int x, int z);
int GetGameEnemyCount();
bool GetGameEnemyPosition(int index, VECTOR& pos, int& layer);
