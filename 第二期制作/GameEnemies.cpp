#include "GameEnemies.h"
#include "MapData.h"
#include "MapLoader.h"
#include <math.h>
#include <vector>

using namespace std;

struct GameEnemy
{
    // 2026-06-12: [ENEMY] CSVから読んだ敵の種類と配置位置を、実行中の敵状態として保持するため追加。
    int id;
    int layer;
    int x;
    int z;
    bool active;
    float cooldown;
};

struct EnemyProjectile
{
    // 2026-06-12: 投げ敵の弾を敵本体とは別に移動・衝突管理するため追加。
    VECTOR pos;
    VECTOR velocity;
    int layer;
    bool active;
};

static vector<GameEnemy> gameEnemies;
static vector<EnemyProjectile> enemyProjectiles;
static float enemyDamageCooldown = 0.0f;

VECTOR GetGameEnemyWorldPosition(int layer, int x, int z)
{
    // 2026-06-12: ENEMYのセル座標を描画・攻撃判定で使うワールド座標へ変換するため追加。
    return VGet(
        CellToWorldCenter(x),
        layer * BLOCK_SIZE + 80.0f,
        CellToWorldCenter(z)
    );
}

static bool IsEnemySpawnPosValid(int layer, int x, int z)
{
    // 2026-06-12: 外部生成時に範囲外セルへ敵を作らないよう追加。
    return layer >= 0 && layer < BLOCK_NUM_Y && x >= 0 && x < BLOCK_NUM_X && z >= 0 && z < BLOCK_NUM_Z;
}

static bool IsEnemyIdValid(int enemyId)
{
    // 2026-06-12: 未定義の敵IDを外部生成で混ぜないため追加。
    return enemyId == GAME_ENEMY_THROW;
}

static float DistanceXZ(VECTOR a, VECTOR b)
{
    float dx = a.x - b.x;
    float dz = a.z - b.z;
    return sqrtf(dx * dx + dz * dz);
}

static bool CanDamagePlayer()
{
    return enemyDamageCooldown <= 0.0f;
}

static void DamagePlayer(bool& playerDamaged)
{
    if (!CanDamagePlayer())
        return;

    playerDamaged = true;
    enemyDamageCooldown = 90.0f;
}

static void SpawnProjectile(const GameEnemy& enemy, VECTOR playerPos)
{
    VECTOR start = GetGameEnemyWorldPosition(enemy.layer, enemy.x, enemy.z);
    start.y += 60.0f;

    float dx = playerPos.x - start.x;
    float dz = playerPos.z - start.z;
    float len = sqrtf(dx * dx + dz * dz);
    if (len < 0.001f)
        return;

    dx /= len;
    dz /= len;

    EnemyProjectile projectile;
    projectile.pos = start;
    projectile.velocity = VGet(dx * 14.0f, 0.0f, dz * 14.0f);
    projectile.layer = enemy.layer;
    projectile.active = true;
    enemyProjectiles.push_back(projectile);
}

static void AddEnemyFromMap(int id, int layer, int x, int z)
{
    // 2026-06-12: GameEnemyMapのIDを実行用GameEnemyへ変換するため追加。
    if (id < 0)
        return;

    GameEnemy enemy;
    enemy.id = id;
    enemy.layer = layer;
    enemy.x = x;
    enemy.z = z;
    enemy.active = true;
    enemy.cooldown = 30.0f;
    gameEnemies.push_back(enemy);
}

bool HasGameEnemyAt(int layer, int x, int z)
{
    // 2026-06-12: 同じセルに有効な敵がいるか確認できるよう追加。
    for (int i = 0; i < (int)gameEnemies.size(); i++)
    {
        const GameEnemy& enemy = gameEnemies[i];
        if (enemy.active && enemy.layer == layer && enemy.x == x && enemy.z == z)
            return true;
    }

    return false;
}

bool TrySpawnGameEnemy(int enemyId, int layer, int x, int z)
{
    // 2026-06-12: debug用。
    if (!IsEnemyIdValid(enemyId) || !IsEnemySpawnPosValid(layer, x, z) || HasGameEnemyAt(layer, x, z))
        return false;

    AddEnemyFromMap(enemyId, layer, x, z);
    return true;
}

bool TrySpawnGameEnemyAtWorld(int enemyId, VECTOR pos)
{
    // 2026-06-12: ワールド座標をセル座標へ変換して敵を生成できるよう追加。
    int layer = GetMapLayerFromWorldY(pos.y + BLOCK_SIZE * 0.5f);
    int x = WorldToCell(pos.x);
    int z = WorldToCell(pos.z);
    return TrySpawnGameEnemy(enemyId, layer, x, z);
}

void SpawnGameEnemy(int enemyId, int layer, int x, int z)
{
    TrySpawnGameEnemy(enemyId, layer, x, z);
}

void InitGameEnemies()
{
    // 2026-06-12: [ENEMY]から読んだGameEnemyMapを、ゲーム開始時の敵リストへ変換するため追加。
    gameEnemies.clear();
    enemyProjectiles.clear();
    enemyDamageCooldown = 0.0f;

    for (int y = 0; y < BLOCK_NUM_Y; y++)
    {
        for (int z = 0; z < BLOCK_NUM_Z; z++)
        {
            for (int x = 0; x < BLOCK_NUM_X; x++)
            {
                AddEnemyFromMap(GameEnemyMap[y][z][x], y, x, z);
            }
        }
    }
}

void UpdateGameEnemies(VECTOR playerPos, int playerLayer, bool& playerDamaged)
{
    // 2026-06-12: 敵AIと敵弾だけを更新し、EVENTギミック処理から分離するため追加。
    if (enemyDamageCooldown > 0.0f)
        enemyDamageCooldown -= 1.0f;

    for (int i = 0; i < (int)gameEnemies.size(); i++)
    {
        GameEnemy& enemy = gameEnemies[i];
        if (!enemy.active)
            continue;

        if (enemy.cooldown > 0.0f)
            enemy.cooldown -= 1.0f;

        if (enemy.id == GAME_ENEMY_THROW)
        {
            float dist = DistanceXZ(GetGameEnemyWorldPosition(enemy.layer, enemy.x, enemy.z), playerPos);
            if (enemy.layer == playerLayer && dist <= BLOCK_SIZE * 4.0f && enemy.cooldown <= 0.0f)
            {
                SpawnProjectile(enemy, playerPos);
                enemy.cooldown = 95.0f;
            }
        }
    }

    for (int i = 0; i < (int)enemyProjectiles.size(); i++)
    {
        EnemyProjectile& projectile = enemyProjectiles[i];
        if (!projectile.active)
            continue;

        projectile.pos.x += projectile.velocity.x;
        projectile.pos.z += projectile.velocity.z;

        if (projectile.layer == playerLayer && DistanceXZ(projectile.pos, playerPos) <= 90.0f)
        {
            projectile.active = false;
            DamagePlayer(playerDamaged);
        }

        int x = WorldToCell(projectile.pos.x);
        int z = WorldToCell(projectile.pos.z);
        if (x < 0 || x >= BLOCK_NUM_X || z < 0 || z >= BLOCK_NUM_Z)
        {
            projectile.active = false;
        }
        else if (!CanMoveWorldPosition(projectile.layer, projectile.pos.x, projectile.pos.z, 30.0f))
        {
            projectile.active = false;
        }
    }
}

void DrawGameEnemies()
{
    // 2026-06-12: 敵本体と敵弾の描画をEVENT描画から分離するため追加。
    for (int i = 0; i < (int)gameEnemies.size(); i++)
    {
        const GameEnemy& enemy = gameEnemies[i];
        if (!enemy.active)
            continue;

        VECTOR pos = GetGameEnemyWorldPosition(enemy.layer, enemy.x, enemy.z);
        int color = GetColor(255, 140, 40);
        DrawSphere3D(pos, 68.0f, 16, color, GetColor(255, 255, 255), TRUE);
    }

    for (int i = 0; i < (int)enemyProjectiles.size(); i++)
    {
        const EnemyProjectile& projectile = enemyProjectiles[i];
        if (!projectile.active)
            continue;

        DrawSphere3D(projectile.pos, 32.0f, 12, GetColor(255, 80, 30), GetColor(255, 255, 255), TRUE);
    }
}

int GetGameEnemyCount()
{
    // 2026-06-12: 他処理が敵数を参照できるようにするため追加。
    return (int)gameEnemies.size();
}

bool GetGameEnemyPosition(int index, VECTOR& pos, int& layer)
{
    // 2026-06-12: 他処理が敵のワールド座標と階層を取得できるようにするため追加。
    if (index < 0 || index >= (int)gameEnemies.size())
        return false;

    const GameEnemy& enemy = gameEnemies[index];
    if (!enemy.active)
        return false;

    pos = GetGameEnemyWorldPosition(enemy.layer, enemy.x, enemy.z);
    layer = enemy.layer;
    return true;
}