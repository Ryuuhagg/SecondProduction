#include "GameObjects.h"
#include "MapData.h"
#include "MapLoader.h"
#include <math.h>
#include <memory>
#include <utility>
#include <vector>

using namespace std;

// 2026-06-02: マップロード後に生成されたゲーム用ギミックの現在状態を保持するため追加。
static int gameObjectKeyCount = 0;
static int gameObjectPickupCount = 0;
static int gameObjectPickupTotal = 0;
static float playerDamageCooldown = 0.0f;
// 2026-06-22: 接触したチェックポイントを死亡後の復帰地点として保持するため追加。
static bool gameCheckpointActivated = false;
static int gameCheckpointLayer = 0;
static int gameCheckpointX = 0;
static int gameCheckpointZ = 0;

static VECTOR GetEventWorldPosition(int layer, int x, int z)
{
    // 2026-06-02: EVENTのセル座標を、描画と接触判定で使うワールド座標へ変換するため追加。
    return VGet(
        x * BLOCK_SIZE + BLOCK_SIZE * 0.5f,
        layer * BLOCK_SIZE + 80.0f,
        z * BLOCK_SIZE + BLOCK_SIZE * 0.5f
    );
}

static float DistanceXZ(VECTOR a, VECTOR b)
{
    // 2026-06-02: ギミック判定は上方向の段差よりマス内距離を優先したいので、XZ距離で見るため追加。
    float dx = a.x - b.x;
    float dz = a.z - b.z;
    return sqrtf(dx * dx + dz * dz);
}

static bool IsSameLayerAsPlayer(int layer, int playerLayer)
{
    // 2026-06-12: ジャンプの高さではなく、階段で確定したプレイヤー階層だけで判定する。
    return layer == playerLayer;
}

static bool CanDamagePlayer()
{
    // 2026-06-02: 罠や扉で連続ヒットして一瞬でライフが消えないよう、無敵時間を作るため追加。
    return playerDamageCooldown <= 0.0f;
}

static void DamagePlayer(bool& playerDamaged)
{
    // 2026-06-02: GameObjects側からPlayerを直接触らず、Sceneへダメージ発生だけ渡すため追加。
    if (!CanDamagePlayer())
        return;

    playerDamaged = true;
    playerDamageCooldown = 90.0f;
}

// 2026-06-17: ギミックごとの処理をif文で増やさず、種類ごとのclassへ分けて管理するため追加。
class GameObjectBase
{
public:
    GameObjectBase(int eventId, int objectLayer, int mapX, int mapZ)
        : id(eventId), layer(objectLayer), x(mapX), z(mapZ), active(true)
    {
    }

    virtual ~GameObjectBase()
    {
    }

    virtual void Update(VECTOR playerPos, int playerLayer, bool& playerDamaged) = 0;
    virtual void Draw() const = 0;

    bool IsActive() const
    {
        return active;
    }

    VECTOR GetWorldPosition() const
    {
        // 2026-06-17: class外の処理が座標だけを安全に読むため追加。
        return GetPosition();
    }

    int GetLayer() const
    {
        // 2026-06-17: class外の処理が階層だけを安全に読むため追加。
        return layer;
    }

protected:
    VECTOR GetPosition() const
    {
        return GetEventWorldPosition(layer, x, z);
    }

    bool IsNearPlayer(VECTOR playerPos, int playerLayer, float radius) const
    {
        // 2026-06-17: 各classが同じ接触判定を使えるよう、基底classにまとめるため追加。
        if (!IsSameLayerAsPlayer(layer, playerLayer))
            return false;

        return DistanceXZ(GetPosition(), playerPos) <= radius;
    }

    int id;
    int layer;
    int x;
    int z;
    bool active;
};

// 2026-06-17: 取るもの専用の取得処理をGameObjectBaseから分けるため追加。
class PickupObject : public GameObjectBase
{
public:
    PickupObject(int objectLayer, int mapX, int mapZ)
        : GameObjectBase(GAME_EVENT_PICKUP, objectLayer, mapX, mapZ)
    {
    }

    void Update(VECTOR playerPos, int playerLayer, bool& playerDamaged) override;
    void Draw() const override;
};

// 2026-06-17: 避けるもの専用のダメージ処理をGameObjectBaseから分けるため追加。
class HazardObject : public GameObjectBase
{
public:
    HazardObject(int objectLayer, int mapX, int mapZ)
        : GameObjectBase(GAME_EVENT_HAZARD, objectLayer, mapX, mapZ)
    {
    }

    void Update(VECTOR playerPos, int playerLayer, bool& playerDamaged) override;
    void Draw() const override;
};

// 2026-06-17: カギ取得の処理を専用classに閉じ込めるため追加。
class KeyObject : public GameObjectBase
{
public:
    KeyObject(int objectLayer, int mapX, int mapZ)
        : GameObjectBase(GAME_EVENT_KEY, objectLayer, mapX, mapZ)
    {
    }

    void Update(VECTOR playerPos, int playerLayer, bool& playerDamaged) override;
    void Draw() const override;
};

// 2026-06-17: カギ扉の開錠と失敗時ダメージを専用classに分けるため追加。
class LockedDoorObject : public GameObjectBase
{
public:
    LockedDoorObject(int objectLayer, int mapX, int mapZ)
        : GameObjectBase(GAME_EVENT_LOCKED_DOOR, objectLayer, mapX, mapZ)
    {
    }

    void Update(VECTOR playerPos, int playerLayer, bool& playerDamaged) override;
    void Draw() const override;
};

// 2026-06-22: class化で抜けていたチェックポイントの表示・接触・復帰地点更新を戻すため追加。
class CheckpointObject : public GameObjectBase
{
public:
    CheckpointObject(int objectLayer, int mapX, int mapZ)
        : GameObjectBase(GAME_EVENT_CHECKPOINT, objectLayer, mapX, mapZ)
    {
    }

    void Update(VECTOR playerPos, int playerLayer, bool& playerDamaged) override;
    void Draw() const override;
};

// 2026-06-17: 種類ごとのclassを同じ配列で扱うため、ポリモーフィズム用のunique_ptrで保持するよう追加。
static vector<unique_ptr<GameObjectBase>> gameObjects;

static unique_ptr<GameObjectBase> CreateGameObjectFromEvent(int id, int layer, int x, int z)
{
    // 2026-06-17: EVENT IDからclassを作り、InitGameObjectsの分岐を小さくするため追加。
    if (id == GAME_EVENT_PICKUP)
        return unique_ptr<GameObjectBase>(new PickupObject(layer, x, z));
    if (id == GAME_EVENT_HAZARD)
        return unique_ptr<GameObjectBase>(new HazardObject(layer, x, z));
    if (id == GAME_EVENT_KEY)
        return unique_ptr<GameObjectBase>(new KeyObject(layer, x, z));
    if (id == GAME_EVENT_LOCKED_DOOR)
        return unique_ptr<GameObjectBase>(new LockedDoorObject(layer, x, z));
    // 2026-06-22: EVENT ID 7を無視せず、ゲーム中に見えるチェックポイントclassへ変換するため追加。
    if (id == GAME_EVENT_CHECKPOINT)
        return unique_ptr<GameObjectBase>(new CheckpointObject(layer, x, z));

    return unique_ptr<GameObjectBase>();
}

void PickupObject::Update(VECTOR playerPos, int playerLayer, bool& playerDamaged)
{
    if (IsNearPlayer(playerPos, playerLayer, 115.0f))
    {
        active = false;
        gameObjectPickupCount++;
    }
}

void PickupObject::Draw() const
{
    DrawSphere3D(GetPosition(), 45.0f, 16, GetColor(80, 220, 255), GetColor(255, 255, 255), TRUE);
}

void HazardObject::Update(VECTOR playerPos, int playerLayer, bool& playerDamaged)
{
    if (IsNearPlayer(playerPos, playerLayer, 120.0f))
    {
        DamagePlayer(playerDamaged);
    }
}

void HazardObject::Draw() const
{
    DrawSphere3D(GetPosition(), 60.0f, 16, GetColor(255, 70, 70), GetColor(255, 255, 255), TRUE);
}

void KeyObject::Update(VECTOR playerPos, int playerLayer, bool& playerDamaged)
{
    if (IsNearPlayer(playerPos, playerLayer, 115.0f))
    {
        active = false;
        gameObjectKeyCount++;
    }
}

void KeyObject::Draw() const
{
    VECTOR pos = GetPosition();
    int color = GetColor(255, 220, 50);
    DrawSphere3D(pos, 42.0f, 16, color, GetColor(255, 255, 255), TRUE);
    DrawLine3D(VGet(pos.x - 55.0f, pos.y, pos.z), VGet(pos.x + 55.0f, pos.y, pos.z), color);
}

void LockedDoorObject::Update(VECTOR playerPos, int playerLayer, bool& playerDamaged)
{
    if (!IsNearPlayer(playerPos, playerLayer, 130.0f))
        return;

    if (gameObjectKeyCount > 0)
    {
        gameObjectKeyCount--;
        active = false;
    }
    else
    {
        DamagePlayer(playerDamaged);
    }
}

void LockedDoorObject::Draw() const
{
    VECTOR pos = GetPosition();
    int color = GetColor(180, 120, 255);
    DrawSphere3D(pos, 75.0f, 16, color, GetColor(255, 255, 255), TRUE);
    DrawCube3D(
        VGet(pos.x - 95.0f, layer * BLOCK_SIZE, pos.z - 30.0f),
        VGet(pos.x + 95.0f, layer * BLOCK_SIZE + 190.0f, pos.z + 30.0f),
        color,
        GetColor(70, 40, 120),
        TRUE
    );
}

void CheckpointObject::Update(VECTOR playerPos, int playerLayer, bool& playerDamaged)
{
    // 2026-06-22: プレイヤーが触れた地点を、次にダメージを受けた時の復帰地点へするため追加。
    if (!IsNearPlayer(playerPos, playerLayer, 130.0f))
        return;

    gameCheckpointActivated = true;
    gameCheckpointLayer = layer;
    gameCheckpointX = x;
    gameCheckpointZ = z;
}

void CheckpointObject::Draw() const
{
    // 2026-06-22: 設置場所と有効状態をゲーム中に判別できるよう、未接触は緑、接触済みは黄色で描画するため追加。
    bool isCurrentCheckpoint = gameCheckpointActivated &&
        gameCheckpointLayer == layer && gameCheckpointX == x && gameCheckpointZ == z;
    int color = isCurrentCheckpoint ? GetColor(255, 220, 60) : GetColor(80, 255, 170);
    VECTOR pos = GetPosition();
    float baseY = layer * BLOCK_SIZE;

    DrawCube3D(
        VGet(pos.x - 18.0f, baseY, pos.z - 18.0f),
        VGet(pos.x + 18.0f, baseY + 180.0f, pos.z + 18.0f),
        color,
        GetColor(255, 255, 255),
        TRUE
    );
    DrawSphere3D(VGet(pos.x, baseY + 205.0f, pos.z), 52.0f, 16, color, GetColor(255, 255, 255), TRUE);
    DrawLine3D(VGet(pos.x - 70.0f, baseY + 135.0f, pos.z), VGet(pos.x + 70.0f, baseY + 135.0f, pos.z), color);
}

void InitGameObjects()
{
    // 2026-06-02: CSVから読んだGameEventMapを、遊ぶたびに初期状態のギミックへ変換するため追加。
    gameObjects.clear();
    gameObjectKeyCount = 0;
    gameObjectPickupCount = 0;
    gameObjectPickupTotal = 0;
    playerDamageCooldown = 0.0f;
    // 2026-06-22: 新しくテストプレイを始めた時に、前回のチェックポイント状態を持ち越さないため追加。
    gameCheckpointActivated = false;

    for (int y = 0; y < BLOCK_NUM_Y; y++)
    {
        for (int z = 0; z < BLOCK_NUM_Z; z++)
        {
            for (int x = 0; x < BLOCK_NUM_X; x++)
            {
                int id = GameEventMap[y][z][x];
                if (id < GAME_EVENT_PICKUP)
                    continue;

                unique_ptr<GameObjectBase> obj = CreateGameObjectFromEvent(id, y, x, z);
                if (!obj)
                    continue;

                if (id == GAME_EVENT_PICKUP)
                    gameObjectPickupTotal++;

                gameObjects.push_back(move(obj));
            }
        }
    }
}

void UpdateGameObjects(VECTOR playerPos, int playerLayer, bool& playerDamaged)
{
    // 2026-06-18: 各ギミックclass自身にUpdateを任せ、種類追加時に大きなif文を増やさない形へ変更。
    if (playerDamageCooldown > 0.0f)
        playerDamageCooldown -= 1.0f;

    for (int i = 0; i < (int)gameObjects.size(); i++)
    {
        if (gameObjects[i]->IsActive())
        {
            gameObjects[i]->Update(playerPos, playerLayer, playerDamaged);
        }
    }
}

void DrawGameObjects()
{
    // 2026-06-18: 各ギミックclass自身にDrawを任せ、描画色や形をclassごとに管理できるよう変更。
    for (int i = 0; i < (int)gameObjects.size(); i++)
    {
        if (gameObjects[i]->IsActive())
        {
            gameObjects[i]->Draw();
        }
    }
}

void DrawGameObjectHUD()
{
    // 2026-06-02: 収集数と鍵数がプレイ中に分かるよう、簡易HUDを表示するため追加。
    DrawFormatString(20, 20, GetColor(255, 255, 255), "ITEM %d/%d", gameObjectPickupCount, gameObjectPickupTotal);
    DrawFormatString(20, 44, GetColor(255, 230, 80), "KEY  %d", gameObjectKeyCount);

    // 2026-06-22: チェックポイントが有効になったことをプレイヤーが確認できるようHUD表示を追加。
    if (gameCheckpointActivated)
        DrawString(20, 116, "CHECKPOINT ACTIVE", GetColor(255, 220, 60));

    if (!IsGameObjectGoalUnlocked())
    {
        DrawString(20, 68, "ゴールにはカギか全アイテムが必要", GetColor(255, 180, 80));
    }
}

bool IsGameObjectGoalUnlocked()
{
    // 2026-06-02: 鍵を持っている、または取るものを全部集めた時だけゴール許可にするため追加。
    return gameObjectKeyCount > 0 || gameObjectPickupCount >= gameObjectPickupTotal;
}

int GetGameObjectKeyCount()
{
    // 2026-06-02: HUDやゴール判定から鍵の所持数を参照するため追加。
    return gameObjectKeyCount;
}

int GetGameObjectPickupCount()
{
    // 2026-06-02: HUDで取った数を表示するため追加。
    return gameObjectPickupCount;
}

int GetGameObjectPickupTotal()
{
    // 2026-06-02: HUDで取るものの総数を表示するため追加。
    return gameObjectPickupTotal;
}

VECTOR GetGameRespawnPosition()
{
    // 2026-06-22: 接触済みチェックポイントがあればその床面へ戻し、未接触ならSTARTへ戻すため変更。
    if (gameCheckpointActivated)
    {
        return VGet(
            gameCheckpointX * BLOCK_SIZE + BLOCK_SIZE * 0.5f,
            gameCheckpointLayer * BLOCK_SIZE,
            gameCheckpointZ * BLOCK_SIZE + BLOCK_SIZE * 0.5f
        );
    }

    return GetStartPosition();
}

void ResetGameCheckpoint()
{
    // 2026-06-22: ゲーム終了後に別マップへチェックポイント状態を持ち越さないため、保持値を初期化するよう変更。
    gameCheckpointActivated = false;
    gameCheckpointLayer = 0;
    gameCheckpointX = 0;
    gameCheckpointZ = 0;
}
