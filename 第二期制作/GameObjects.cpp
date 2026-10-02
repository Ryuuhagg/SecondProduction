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
// 2026-07-01: 結合前のチェックポイント復帰状態を戻し、デンジャー死亡後の復帰先を保持する。
static bool gameCheckpointActivated = false;
static int gameCheckpointLayer = 0;
static int gameCheckpointX = 0;
static int gameCheckpointZ = 0;
// 2026-07-03: チェックポイントを専用MV1モデルで描画するためのハンドル。
static int checkpointModelHandle = -1;
// 2026-07-21: 鍵扉を消さずに開いた見た目へ切り替えるため、開き扉モデルを専用に読む。
static int openDoorModelHandle = -1;
// 2026-07-21: トラップ発動後にindicatorからスパイクモデルへ切り替えるため、発動後モデルを専用に読む。
static int triggeredTrapModelHandle = -1;

static VECTOR GetEventWorldPosition(int layer, int x, int z)
{
    // 2026-06-02: EVENTのセル座標を、描画と接触判定で使うワールド座標へ変換するため追加。
    return VGet(
        CellToWorldCenter(x),
        layer * BLOCK_SIZE + 80.0f,
        CellToWorldCenter(z)
    );
}

static float DistanceXZ(VECTOR a, VECTOR b)
{
    // 2026-06-02: ギミック判定は上方向の段差よりマス内距離を優先したいので、XZ距離で見るため追加。
    float dx = a.x - b.x;
    float dz = a.z - b.z;
    return sqrtf(dx * dx + dz * dz);
}

static float DistancePointToSegmentXZ(float px, float pz, float x1, float z1, float x2, float z2, float& along)
{
    // 2026-07-22: 鍵扉の当たり判定を壁と同じ「セル端の線」へ合わせるため、線分への距離を計算する。
    float dx = x2 - x1;
    float dz = z2 - z1;
    float lenSq = dx * dx + dz * dz;
    along = 0.0f;
    if (lenSq > 0.0001f)
    {
        along = ((px - x1) * dx + (pz - z1) * dz) / lenSq;
        if (along < 0.0f) along = 0.0f;
        if (along > 1.0f) along = 1.0f;
    }

    float nearestX = x1 + dx * along;
    float nearestZ = z1 + dz * along;
    float diffX = px - nearestX;
    float diffZ = pz - nearestZ;
    return sqrtf(diffX * diffX + diffZ * diffZ);
}

static bool HitLockedDoorWallLine(int x, int z, int rot, float worldX, float worldZ, float radius)
{
    // 2026-07-22: 鍵扉は丸判定ではなく、描画位置と同じセル端を壁1枚として判定する。
    float left = x * BLOCK_SIZE;
    float right = (x + 1) * BLOCK_SIZE;
    float top = z * BLOCK_SIZE;
    float bottom = (z + 1) * BLOCK_SIZE;

    float x1 = left;
    float z1 = top;
    float x2 = right;
    float z2 = top;
    switch (rot & 3)
    {
    case 0:
        x1 = left;  z1 = top;
        x2 = right; z2 = top;
        break;
    case 1:
        x1 = right; z1 = top;
        x2 = right; z2 = bottom;
        break;
    case 2:
        x1 = left;  z1 = bottom;
        x2 = right; z2 = bottom;
        break;
    case 3:
        x1 = left; z1 = top;
        x2 = left; z2 = bottom;
        break;
    }

    float lineDX = x2 - x1;
    float lineDZ = z2 - z1;
    float lineLen = sqrtf(lineDX * lineDX + lineDZ * lineDZ);
    float rawAlong = 0.0f;
    if (lineLen > 0.0001f)
        rawAlong = ((worldX - x1) * lineDX + (worldZ - z1) * lineDZ) / (lineLen * lineLen);

    float endMarginRate = lineLen > 0.0001f ? (radius * 0.85f) / lineLen : 0.0f;
    if (rawAlong < -endMarginRate || rawAlong > 1.0f + endMarginRate)
        return false;

    float along = 0.0f;
    float dist = DistancePointToSegmentXZ(worldX, worldZ, x1, z1, x2, z2, along);

    float signedDist = 0.0f;
    switch (rot & 3)
    {
    case 0: signedDist = worldZ - z1; break;
    case 1: signedDist = x1 - worldX; break;
    case 2: signedDist = z1 - worldZ; break;
    case 3: signedDist = worldX - x1; break;
    }

    float frontThickness = BLOCK_SIZE * 0.19f;
    float backThickness = BLOCK_SIZE * 0.03f;
    if (signedDist >= 0.0f)
        return dist <= radius + frontThickness;

    float backRadius = radius * 0.75f;
    return dist <= backRadius + backThickness;
}
static void DrawModelWithoutLighting(int modelHandle)
{
    // 2026-07-21: このDxLibにはMV1SetMaterialUseLightingが無いため、モデル描画中だけライトを切って全体を明るく見せる。
    int oldLighting = GetLightEnable();
    SetUseLighting(FALSE);
    SetLightEnable(FALSE);
    MV1DrawModel(modelHandle);
    SetLightEnable(oldLighting);
    SetUseLighting(TRUE);
}

static void ApplyLockedDoorDrawOffset(VECTOR& modelPos, int rot)
{
    // 2026-07-21: 鍵扉はEVENT中央ではなく壁と同じセル辺に置き、壁穴と位置を合わせる。
    const float halfBlock = BLOCK_SIZE * 0.5f;
    switch (rot & 3)
    {
    case 0: modelPos.z -= halfBlock; break;
    case 1: modelPos.x += halfBlock; break;
    case 2: modelPos.z += halfBlock; break;
    case 3: modelPos.x -= halfBlock; break;
    }
}

static void DrawGameEventModelHandle(int modelHandle, int layer, int x, int z, int rot, bool adjustLockedDoor)
{
    // 2026-07-21: 閉じ扉/開き扉の両方を同じ座標と回転で描けるよう、EVENTモデル描画を共通化する。
    VECTOR modelPos = VGet(
        CellToWorldCenter(x),
        layer * BLOCK_SIZE + 8.0f,
        CellToWorldCenter(z)
    );
    if (adjustLockedDoor)
    {
        ApplyLockedDoorDrawOffset(modelPos, rot);
    }
    MV1SetPosition(modelHandle, modelPos);
    MV1SetRotationXYZ(modelHandle, VGet(0.0f, (rot & 3) * DX_PI_F * 0.5f, 0.0f));
    DrawModelWithoutLighting(modelHandle);
}

static bool DrawConfiguredGameEventModel(int eventId, int layer, int x, int z, int rot)
{
    int modelId = GetGameEventModelId(eventId);
    if (modelId < 0 || modelId >= MODEL_MAX || gameModelHandles[modelId] == -1)
        return false;

    // 2026-07-06: model_config.csvでEVENTにモデルが設定されていれば、ゲーム中も同じ見た目で描く。
    DrawGameEventModelHandle(gameModelHandles[modelId], layer, x, z, rot, eventId == GAME_EVENT_LOCKED_DOOR);
    return true;
}

static bool IsSameLayerAsPlayer(int layer, VECTOR playerPos)
{
    // 2026-06-17: class化後も別階層のギミックに触れた扱いにならないよう、階層判定を共通化するため追加。
    return layer == GetMapLayerFromWorldY(playerPos.y + BLOCK_SIZE * 0.5f);
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
        : id(eventId), layer(objectLayer), x(mapX), z(mapZ), rot(0), active(true)
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

    int GetId() const
    {
        // 2026-07-21: 鍵扉だけを移動阻止対象にするため、外側からイベントIDを確認できるようにする。
        return id;
    }

    int GetRot() const
    {
        // 2026-07-22: 鍵扉の壁型当たり判定で、配置時の回転を使うため追加。
        return rot;
    }

    virtual bool BlocksMovement() const
    {
        // 2026-07-21: 開いた鍵扉は描画したまま通れるよう、移動阻止するかをclass側で返す。
        return active;
    }

    void SetRot(int eventRot)
    {
        // 2026-07-21: CSVから読んだEVENT回転を、生成後の各ギミックへ渡すため追加。
        rot = eventRot & 3;
    }

protected:
    VECTOR GetPosition() const
    {
        return GetEventWorldPosition(layer, x, z);
    }

    bool IsNearPlayer(VECTOR playerPos, float radius) const
    {
        // 2026-06-17: 各classが同じ接触判定を使えるよう、基底classにまとめるため追加。
        if (!IsSameLayerAsPlayer(layer, playerPos))
            return false;

        return DistanceXZ(GetPosition(), playerPos) <= radius;
    }

    int id;
    int layer;
    int x;
    int z;
    // 2026-07-21: EVENTモデルをCSVで保存した向きのまま描画するため、各ギミックが回転値を持つ。
    int rot;
    bool active;
};

// 2026-07-06: START/GOALなど、動きは不要でモデルだけ表示したいEVENT用に追加。
class VisualEventObject : public GameObjectBase
{
public:
    VisualEventObject(int eventId, int objectLayer, int mapX, int mapZ)
        : GameObjectBase(eventId, objectLayer, mapX, mapZ)
    {
    }

    void Update(VECTOR playerPos, int playerLayer, bool& playerDamaged) override
    {
    }

    void Draw() const override
    {
        DrawConfiguredGameEventModel(id, layer, x, z, rot);
    }
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
        : GameObjectBase(GAME_EVENT_HAZARD, objectLayer, mapX, mapZ), triggered(false)
    {
    }

    void Update(VECTOR playerPos, int playerLayer, bool& playerDamaged) override;
    void Draw() const override;

private:
    // 2026-07-21: トラップは一度だけ発動し、発動後はスパイクモデル表示に切り替える。
    bool triggered;
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
        : GameObjectBase(GAME_EVENT_LOCKED_DOOR, objectLayer, mapX, mapZ), opened(false)
    {
    }

    void Update(VECTOR playerPos, int playerLayer, bool& playerDamaged) override;
    void Draw() const override;
    bool BlocksMovement() const override;

private:
    // 2026-07-21: 鍵で開けたあとも扉オブジェクトを残し、開き扉モデルへ切り替えるため追加。
    bool opened;
};

// 2026-07-01: 結合前にあったチェックポイントの接触・描画処理を戻す。
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
    if (id == GAME_EVENT_START || id == GAME_EVENT_GOAL)
        return unique_ptr<GameObjectBase>(new VisualEventObject(id, layer, x, z));
    if (id == GAME_EVENT_PICKUP)
        return unique_ptr<GameObjectBase>(new PickupObject(layer, x, z));
    if (id == GAME_EVENT_HAZARD)
        return unique_ptr<GameObjectBase>(new HazardObject(layer, x, z));
    if (id == GAME_EVENT_KEY)
        return unique_ptr<GameObjectBase>(new KeyObject(layer, x, z));
    if (id == GAME_EVENT_LOCKED_DOOR)
        return unique_ptr<GameObjectBase>(new LockedDoorObject(layer, x, z));

    // 2026-07-01: EVENT ID 7を無視せず、チェックポイントとして生成する。
    if (id == GAME_EVENT_CHECKPOINT)
        return unique_ptr<GameObjectBase>(new CheckpointObject(layer, x, z));

    return unique_ptr<GameObjectBase>();
}

void PickupObject::Update(VECTOR playerPos, int playerLayer, bool& playerDamaged)
{
    if (IsNearPlayer(playerPos, 115.0f))
    {
        active = false;
        gameObjectPickupCount++;
    }
}

void PickupObject::Draw() const
{
    if (DrawConfiguredGameEventModel(id, layer, x, z, rot))
        return;

    // 2026-07-21: モデル未設定時の仮表示を出さず、登録モデルだけを見せる。
}

void HazardObject::Update(VECTOR playerPos, int playerLayer, bool& playerDamaged)
{
    if (playerLayer != layer)
        return;

    float left = x * BLOCK_SIZE;
    float right = (x + 1) * BLOCK_SIZE;
    float front = z * BLOCK_SIZE;
    float back = (z + 1) * BLOCK_SIZE;
    if (playerPos.x >= left && playerPos.x <= right && playerPos.z >= front && playerPos.z <= back)
    {
        DamagePlayer(playerDamaged);
        if (!triggered)
        {
            // 2026-07-21: 初回接触でindicatorからスパイクへ切り替え、発動後も出ているトラップとしてダメージ判定を残す。
            triggered = true;
        }
    }
}

void HazardObject::Draw() const
{
    if (triggered)
    {
        if (triggeredTrapModelHandle != -1)
        {
            // 2026-07-21: 発動後はindicator-special-crossではなくトラップ本体モデルを表示する。
            DrawGameEventModelHandle(triggeredTrapModelHandle, layer, x, z, rot, false);
        }
        return;
    }

    if (DrawConfiguredGameEventModel(id, layer, x, z, rot))
        return;

    // 2026-07-21: モデル未設定時の仮表示を出さず、登録モデルだけを見せる。
}

void KeyObject::Update(VECTOR playerPos, int playerLayer, bool& playerDamaged)
{
    if (IsNearPlayer(playerPos, 115.0f))
    {
        active = false;
        gameObjectKeyCount++;
    }
}

void KeyObject::Draw() const
{
    if (DrawConfiguredGameEventModel(id, layer, x, z, rot))
        return;

    // 2026-07-21: 鍵モデルを使うため、仮表示の黄色い丸と線は出さない。
}

void LockedDoorObject::Update(VECTOR playerPos, int playerLayer, bool& playerDamaged)
{
    if (opened)
        return;

    if (playerLayer != layer)
        return;

    if (!HitLockedDoorWallLine(x, z, rot, playerPos.x, playerPos.z, 40.0f))
        return;

    if (gameObjectKeyCount > 0)
    {
        gameObjectKeyCount--;
        // 2026-07-21: 鍵扉は消さずに開いた状態へ変え、見た目を開き扉モデルへ切り替える。
        opened = true;
    }
    else
    {
        DamagePlayer(playerDamaged);
    }
}

void LockedDoorObject::Draw() const
{
    if (opened && openDoorModelHandle != -1)
    {
        // 2026-07-21: 開錠後は消さず、開いた扉モデルを同じマス・同じ向きで描画する。
        DrawGameEventModelHandle(openDoorModelHandle, layer, x, z, rot, true);
        return;
    }

    if (DrawConfiguredGameEventModel(id, layer, x, z, rot))
        return;

    // 2026-07-21: 鍵扉もモデル未設定時の紫の仮表示を出さず、登録モデルだけを見せる。
}

bool LockedDoorObject::BlocksMovement() const
{
    // 2026-07-21: 開いた鍵扉はモデルを残したまま通行可能にする。
    return active && !opened;
}

void CheckpointObject::Update(VECTOR playerPos, int playerLayer, bool& playerDamaged)
{
    // 2026-07-01: プレイヤーが触れた地点を、次にダメージを受けた時の復帰地点として保持する。
    if (!IsNearPlayer(playerPos, 130.0f))
        return;

    gameCheckpointActivated = true;
    gameCheckpointLayer = layer;
    gameCheckpointX = x;
    gameCheckpointZ = z;
}

void CheckpointObject::Draw() const
{
    if (DrawConfiguredGameEventModel(id, layer, x, z, rot))
        return;

    // 2026-07-21: チェックポイント判定と旗の位置を合わせるため、通常EVENTと同じマス中央描画に統一する。
    if (checkpointModelHandle != -1)
    {
        DrawGameEventModelHandle(checkpointModelHandle, layer, x, z, rot, false);
    }
}

void InitGameObjects()
{
    // 2026-06-02: CSVから読んだGameEventMapを、遊ぶたびに初期状態のギミックへ変換するため追加。
    gameObjects.clear();
    gameObjectKeyCount = 0;
    gameObjectPickupCount = 0;
    gameObjectPickupTotal = 0;
    playerDamageCooldown = 0.0f;
    // 2026-07-01: 新しく遊び始めた時に、前回のチェックポイントを持ち越さない。
    gameCheckpointActivated = false;
    // 2026-07-21: チェックポイント表示用の予備モデルも、最新のKenney旗モデルへ差し替える。
    if (checkpointModelHandle != -1)
    {
        MV1DeleteModel(checkpointModelHandle);
        checkpointModelHandle = -1;
    }
    checkpointModelHandle = MV1LoadModel("model/3Dmodels/ギミック/platformer/flag.mv1");
    if (checkpointModelHandle != -1)
    {
        MV1SetScale(checkpointModelHandle, VGet(4.0f, 4.0f, 4.0f));
    }

    if (openDoorModelHandle != -1)
    {
        MV1DeleteModel(openDoorModelHandle);
        openDoorModelHandle = -1;
    }
    // 2026-07-21: 鍵扉が開いた後の見た目用モデル。配置IDは増やさず、ゲーム中だけ使う。
    openDoorModelHandle = MV1LoadModel("model/3Dmodels/ギミック/platformer/door-large-open.mv1");
    if (openDoorModelHandle != -1)
    {
        MV1SetScale(openDoorModelHandle, VGet(4.0f, 4.0f, 4.0f));
    }

    if (triggeredTrapModelHandle != -1)
    {
        MV1DeleteModel(triggeredTrapModelHandle);
        triggeredTrapModelHandle = -1;
    }
    // 2026-07-21: トラップ発動後はスパイクモデルへ切り替えて、発動済みが見た目で分かるようにする。
    triggeredTrapModelHandle = MV1LoadModel("model/3Dmodels/ギミック/platformer/trap-spikes-large.mv1");
    if (triggeredTrapModelHandle != -1)
    {
        MV1SetScale(triggeredTrapModelHandle, VGet(4.0f, 4.0f, 4.0f));
    }

    bool hasStartEventObject = false;
    bool hasGoalEventObject = false;

    for (int y = 0; y < BLOCK_NUM_Y; y++)
    {
        for (int z = 0; z < BLOCK_NUM_Z; z++)
        {
            for (int x = 0; x < BLOCK_NUM_X; x++)
            {
                int id = GameEventMap[y][z][x];
                if (id < 0)
                    continue;

                if (id == GAME_EVENT_START)
                    hasStartEventObject = true;
                else if (id == GAME_EVENT_GOAL)
                    hasGoalEventObject = true;

                unique_ptr<GameObjectBase> obj = CreateGameObjectFromEvent(id, y, x, z);
                if (!obj)
                    continue;

                obj->SetRot(GameEventRot[y][z][x]);
                // 2026-07-21: [EVENT]の回転列をGameObjectへ渡し、タイトルから遊んでも扉の向きを保つ。

                if (id == GAME_EVENT_PICKUP)
                    gameObjectPickupTotal++;

                gameObjects.push_back(move(obj));
            }
        }
    }

    // 2026-07-06: 古いCSVで[EVENT]にSTART/GOAL行が無くても、座標情報から表示モデルを補完する。
    if (!hasStartEventObject)
        gameObjects.push_back(unique_ptr<GameObjectBase>(new VisualEventObject(GAME_EVENT_START, gameStartY, gameStartX, gameStartZ)));
    if (!hasGoalEventObject)
        gameObjects.push_back(unique_ptr<GameObjectBase>(new VisualEventObject(GAME_EVENT_GOAL, gameGoalY, gameGoalX, gameGoalZ)));
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

bool HitActiveLockedDoorObject(int checkLayer, float worldX, float worldZ, float radius)
{
    // 2026-07-21: activeな鍵扉を壁扱いにして、鍵を使って消えるまで通り抜けできないようにする。
    if (gameObjectKeyCount > 0)
    {
        // 2026-07-21: 鍵を持っている時は移動判定で止めず、LockedDoorObject::Updateの開錠処理まで近づけるようにする。
        return false;
    }

    for (int i = 0; i < (int)gameObjects.size(); i++)
    {
        if (!gameObjects[i]->IsActive())
            continue;
        if (gameObjects[i]->GetLayer() != checkLayer)
            continue;
        if (gameObjects[i]->GetId() != GAME_EVENT_LOCKED_DOOR)
            continue;
        if (!gameObjects[i]->BlocksMovement())
            continue;

        VECTOR pos = gameObjects[i]->GetWorldPosition();
        int cellX = WorldToCell(pos.x);
        int cellZ = WorldToCell(pos.z);
        if (HitLockedDoorWallLine(cellX, cellZ, gameObjects[i]->GetRot(), worldX, worldZ, radius))
        {
            // 2026-07-22: 扉の移動阻止も壁と同じセル端ラインで行い、丸判定のズレをなくす。
            return true;
        }
    }

    return false;
}
void DrawGameObjects()
{
    // 2026-06-18: 各ギミックclass自身にDrawを任せ、描画色や形をclassごとに管理できるよう変更。
    int drawLayer = GetGameDrawLayer();
    for (int i = 0; i < (int)gameObjects.size(); i++)
    {
        if (gameObjects[i]->IsActive() && gameObjects[i]->GetLayer() == drawLayer)
        {
            // 2026-07-15: マップの階層フェードに合わせ、ギミックも現在描画中の1層だけ表示する。
            gameObjects[i]->Draw();
        }
    }
}

void DrawGameObjectHUD()
{
    // 2026-06-02: 収集数と鍵数がプレイ中に分かるよう、簡易HUDを表示するため追加。
    DrawFormatString(20, 20, GetColor(255, 255, 255), "ITEM %d/%d", gameObjectPickupCount, gameObjectPickupTotal);
    DrawFormatString(20, 44, GetColor(255, 230, 80), "KEY  %d", gameObjectKeyCount);

    // 2026-07-01: チェックポイントが有効になったことをHUDで確認できるよう戻す。
    if (gameCheckpointActivated)
        DrawString(20, 116, "CHECKPOINT ACTIVE", GetColor(255, 220, 60));
}

bool IsGameObjectGoalUnlocked()
{
    // 2026-07-22: Pickups and keys no longer gate the goal; reaching the goal cell is enough.
    return true;
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
    // 2026-07-01: 結合前と同じく、接触済みチェックポイントがあればそこへ、未接触ならSTARTへ戻す。
    if (gameCheckpointActivated)
    {
        return VGet(
            CellToWorldCenter(gameCheckpointX),
            gameCheckpointLayer * BLOCK_SIZE,
            CellToWorldCenter(gameCheckpointZ)
        );
    }

    return GetStartPosition();
}

void ResetGameCheckpoint()
{
    // 2026-07-01: 別マップや次回プレイへチェックポイント状態を持ち越さない。
    gameCheckpointActivated = false;
    gameCheckpointLayer = 0;
    gameCheckpointX = 0;
    gameCheckpointZ = 0;
}




