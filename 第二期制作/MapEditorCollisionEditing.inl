#pragma region ===== エディター配置/当たり判定補助 =====

static int editorEventIdFromModel[MODEL_MAX];
static int editorEventModelFromEventId[MODEL_MAX];
static int editorEnemyIdFromModel[MODEL_MAX];

static vector<string> SplitCollisionCSV(const string& line)
{
    // 2026-05-11: CSV設定を簡単に増やせるよう、カンマ区切りを共通処理にする。
    vector<string> cols;
    string item;
    stringstream ss(line);
    while (getline(ss, item, ','))
        cols.push_back(item);
    return cols;
}


static bool HasCsvValue(const vector<string>& cols, int index)
{
    return index >= 0 && index < (int)cols.size() && !cols[index].empty() && cols[index] != "-";
}
static float RotToRad(int rot)
{
    // 2026-07-21: 復元時、エディター描画で使う4方向回転のラジアン変換を戻す。
    return (rot & 3) * DX_PI_F * 0.5f;
}

static int GetCollisionEdgeBit(int dir)
{
    // 2026-07-21: 復元時、辺単位の当たり判定を4bitで扱う処理を戻す。
    return 1 << (dir & 3);
}

static bool IsEditorEnemyModelId(int modelId)
{
    return modelId >= 0 && modelId < MODEL_MAX && tabModelCount[ENEMY] > 0;
}

static bool IsEditorUniqueStartGoalEventId(int eventId)
{
    return eventId == 0 || eventId == 1;
}

static void ClearEditorStartGoalCoordForEventId(int eventId)
{
    if (eventId == 0)
    {
        startX = -1;
        startY = -1;
        startZ = -1;
    }
    else if (eventId == 1)
    {
        goalX = -1;
        goalY = -1;
        goalZ = -1;
    }
}

static void ClearEditorEventId(int eventId)
{
    if (!IsEditorUniqueStartGoalEventId(eventId))
        return;

    for (int y = 0; y < BLOCK_NUM_Y; y++)
    {
        for (int zz = 0; zz < BLOCK_NUM_Z; zz++)
        {
            for (int xx = 0; xx < BLOCK_NUM_X; xx++)
            {
                if (EventMap[y][zz][xx] == eventId)
                {
                    EventMap[y][zz][xx] = -1;
                    EventRot[y][zz][xx] = 0;
                }
            }
        }
    }

    ClearEditorStartGoalCoordForEventId(eventId);
}

static void PlaceEventWithRot(int layer, int z, int x, int eventId, int rot)
{
    // 2026-07-22: START/GOAL are unique editor markers, so placing or pasting one moves the old marker.
    if (layer < 0 || layer >= BLOCK_NUM_Y || z < 0 || z >= BLOCK_NUM_Z || x < 0 || x >= BLOCK_NUM_X)
        return;

    if (IsEditorUniqueStartGoalEventId(EventMap[layer][z][x]))
        ClearEditorStartGoalCoordForEventId(EventMap[layer][z][x]);

    if (IsEditorUniqueStartGoalEventId(eventId))
        ClearEditorEventId(eventId);

    EventMap[layer][z][x] = eventId;
    EventRot[layer][z][x] = IsEditorUniqueStartGoalEventId(eventId) ? 0 : (rot & 3);

    if (eventId == 0)
    {
        startX = x;
        startY = layer;
        startZ = z;
    }
    else if (eventId == 1)
    {
        goalX = x;
        goalY = layer;
        goalZ = z;
    }
}

static void PlaceEvent(int layer, int z, int x, int eventId)
{
    // 2026-07-21: EVENTタブ配置時に、CSVのplaceIdへ変換したイベントIDを保存する。
    // 2026-07-21: 扉などのEVENTモデルをRキーで回転して置けるよう、配置時の向きを保存する。
    PlaceEventWithRot(layer, z, x, eventId, currentRot);
}

static int GetCollisionBoxDragMode(int layer, int z, int x, float worldX, float worldZ)
{
    // 2026-07-21: 復元時、BoxCollider風編集のつかむ場所判定を戻す。
    float cx = CellToWorldCenter(x) + CollisionBoxOffsetXMap[layer][z][x];
    float cz = CellToWorldCenter(z) + CollisionBoxOffsetZMap[layer][z][x];
    float hw = CollisionBoxSizeXMap[layer][z][x] * 0.5f;
    float hd = CollisionBoxSizeZMap[layer][z][x] * 0.5f;
    float grab = 35.0f;

    if (fabsf(worldX - (cx - hw)) < grab) return 1;
    if (fabsf(worldX - (cx + hw)) < grab) return 2;
    if (fabsf(worldZ - (cz - hd)) < grab) return 3;
    if (fabsf(worldZ - (cz + hd)) < grab) return 4;
    return 5;
}

static void ApplyCollisionBoxDrag(int layer, int z, int x, int mode, float worldX, float worldZ)
{
    // 2026-07-21: 復元時、BOXの中心移動と各辺サイズ調整を戻す。
    if (layer < 0 || layer >= BLOCK_NUM_Y || z < 0 || z >= BLOCK_NUM_Z || x < 0 || x >= BLOCK_NUM_X)
        return;

    float baseX = CellToWorldCenter(x);
    float baseZ = CellToWorldCenter(z);
    int minSize = 40;
    int maxSize = (int)BLOCK_SIZE;

    if (mode == 5)
    {
        CollisionBoxOffsetXMap[layer][z][x] = (int)(worldX - baseX);
        CollisionBoxOffsetZMap[layer][z][x] = (int)(worldZ - baseZ);
        return;
    }

    float cx = baseX + CollisionBoxOffsetXMap[layer][z][x];
    float cz = baseZ + CollisionBoxOffsetZMap[layer][z][x];
    if (mode == 1 || mode == 2)
    {
        int newSize = (int)(fabsf(worldX - cx) * 2.0f);
        CollisionBoxSizeXMap[layer][z][x] = max(minSize, min(maxSize, newSize));
    }
    else if (mode == 3 || mode == 4)
    {
        int newSize = (int)(fabsf(worldZ - cz) * 2.0f);
        CollisionBoxSizeZMap[layer][z][x] = max(minSize, min(maxSize, newSize));
    }
}

static void DrawManualCollisionBox(int layer, int z, int x, int color)
{
    // 2026-07-21: 復元時、手動BOX当たり判定を3D線で確認できる表示を戻す。
    float cx =
        CellToWorldCenter(x) +
        CollisionBoxOffsetXMap[layer][z][x];

    float cz =
        CellToWorldCenter(z) +
        CollisionBoxOffsetZMap[layer][z][x];
    float yy = layer * BLOCK_SIZE + 26.0f;
    float hw = CollisionBoxSizeXMap[layer][z][x] * 0.5f;
    float hd = CollisionBoxSizeZMap[layer][z][x] * 0.5f;

    DrawLine3D(VGet(cx - hw, yy, cz - hd), VGet(cx + hw, yy, cz - hd), color);
    DrawLine3D(VGet(cx + hw, yy, cz - hd), VGet(cx + hw, yy, cz + hd), color);
    DrawLine3D(VGet(cx + hw, yy, cz + hd), VGet(cx - hw, yy, cz + hd), color);
    DrawLine3D(VGet(cx - hw, yy, cz + hd), VGet(cx - hw, yy, cz - hd), color);
}
static int ParseModelTab(const string& text)
{
    // 2026-05-20: model_config.csv のタブ名から、エディターの配置タブへ変換するため追加。
    if (text == "FLOOR") return FLOOR;
    if (text == "WALL") return WALL;
    if (text == "CORNER") return CORNER;
    if (text == "DECO") return DECO;
    if (text == "ENEMY") return ENEMY;
    if (text == "EVENT") return EVENT;
    return -1;
}

static EditorCollisionType ParseEditorCollisionType(const string& text)
{
    // 2026-05-11: collision_config.csv の文字列から当たり判定タイプを選べるよう追加。
    if (text == "CIRCLE") return EDITOR_COLL_CIRCLE;
    if (text == "BOX") return EDITOR_COLL_BOX;
    if (text == "ARC") return EDITOR_COLL_ARC;
    if (text == "WALL") return EDITOR_COLL_WALL;
    if (text == "STAIRS") return EDITOR_COLL_STAIRS;
    return EDITOR_COLL_NONE;
}

static void ResetEditorPlacementModelMaps()
{
    // 2026-07-06: EVENT/ENEMYの配置IDをmodel_config.csvから引けるよう、読み込み前に対応表を初期化する。
    for (int i = 0; i < MODEL_MAX; i++)
    {
        editorEventIdFromModel[i] = i;
        editorEventModelFromEventId[i] = -1;
        editorEnemyIdFromModel[i] = 0;
    }
}

static bool IsEditorPlacementDisabled(int tab, int id)
{
    // 2026-07-21: 復元時、モデル登録をタブ一覧へ出すか判断する入口を戻す。
    // 2026-07-21: はしごID30は上昇ヒント専用モデルなので、エディターの配置パレットには出さない。
    return id == 30;
}

static void AddModelToEditorTab(int tab, int id)
{
    if (tab < 0 || tab >= TAB_MAX || id < 0 || id >= MODEL_MAX)
        return;
    if (tabModelCount[tab] >= 16)
        return;

    for (int i = 0; i < tabModelCount[tab]; i++)
        if (tabModelList[tab][i] == id)
            return;

    tabModelList[tab][tabModelCount[tab]++] = id;
}

static int GetEditorEventIdFromModel(int modelId)
{
    if (modelId < 0 || modelId >= MODEL_MAX)
        return 0;
    return editorEventIdFromModel[modelId];
}

static int GetEditorModelIdFromEventId(int eventId)
{
    if (eventId < 0 || eventId >= MODEL_MAX)
        return -1;
    return editorEventModelFromEventId[eventId];
}

static int GetEditorEnemyIdFromModel(int modelId)
{
    if (modelId < 0 || modelId >= MODEL_MAX)
        return 0;
    return editorEnemyIdFromModel[modelId];
}

static bool IsEditorStairsId(int id)
{
    return id == 5;
}

static int GetEditorStairsDrawRot(int rot)
{
    return rot & 3;
}

static int GetEditorModelDrawRot(int tab, int id, int rot)
{
    // 2026-07-22: モデル31/34はいったん外したため、追加壁33だけ壁辺に沿うよう90度分だけ補正する。
    if (tab == WALL && id == 33)
        return (rot + 1) & 3;
    // 2026-06-29: 壁は配置した辺に対してモデルだけ内側を向くよう補正する。
    if (tab == WALL)
        return (rot + 2) & 3;
    // 2026-07-22: モデル28はいったん外したため、角モデル29だけ描画時の回転補正を残す。
    if (tab == CORNER && id == 29)
        return (rot + 1) & 3;
    if (tab == DECO && IsEditorStairsId(id))
        return GetEditorStairsDrawRot(rot);
    return rot & 3;
}

static VECTOR GetBlockCenterPosition(int x, int y, int z)
{
    return VGet(
        CellToWorldCenter(x),
        y * BLOCK_SIZE,
        CellToWorldCenter(z)
    );
}

static VECTOR GetWallPosition(int x, int y, int z, int rot)
{
    float px = CellToWorldCenter(x);
    float py = y * BLOCK_SIZE;
    float pz = CellToWorldCenter(z);

    switch (rot & 3)
    {
    case 0: pz = z * BLOCK_SIZE; break;
    case 1: px = (x + 1) * BLOCK_SIZE; break;
    case 2: pz = (z + 1) * BLOCK_SIZE; break;
    case 3: px = x * BLOCK_SIZE; break;
    }

    return VGet(px, py, pz);
}

static VECTOR ApplyEditorModelConfigOffset(int tab, int id, VECTOR pos)
{
    // 2026-07-21: model_config.csvのoffsetX/offsetZ/offsetYで、モデル原点のズレを見ながら調整できるようにする。
    if (id >= 0 && id < MODEL_MAX)
    {
        pos.x += modelOffsetX[id];
        pos.z += modelOffsetZ[id];
        pos.y += modelOffsetY[id];
    }
    return pos;
}

static VECTOR ApplyEditorWallModelAlignmentOffset(int tab, int id, int rot, VECTOR pos)
{
    // 2026-07-22: モデル34はいったん外したため、追加壁の個別位置補正は使わない。
    return pos;
}

static VECTOR GetModelDrawPosition(int tab, int x, int y, int z, int rot)
{
    if (tab == WALL)
        return GetWallPosition(x, y, z, rot);
    return GetBlockCenterPosition(x, y, z);
}

static void ResetCollisionBox(int layer, int z, int x)
{
    // 2026-05-20: BoxCollider風編集値を標準サイズへ戻す。
    if (layer < 0 || layer >= BLOCK_NUM_Y || z < 0 || z >= BLOCK_NUM_Z || x < 0 || x >= BLOCK_NUM_X)
        return;
    CollisionBoxOffsetXMap[layer][z][x] = 0;
    CollisionBoxOffsetZMap[layer][z][x] = 0;
    CollisionBoxSizeXMap[layer][z][x] = 200;
    CollisionBoxSizeZMap[layer][z][x] = 200;
}

static void InitEditorCollisionTable()
{
    // 2026-05-11: 既定当たり判定をコード初期値とCSVで設定する。
    for (int i = 0; i < MODEL_MAX; i++)
    {
        editorCollisionTable[i].type = EDITOR_COLL_NONE;
        editorCollisionTable[i].radius = 0.0f;
        editorCollisionTable[i].width = 0.0f;
        editorCollisionTable[i].depth = 0.0f;
    }

    editorCollisionTable[1].type = EDITOR_COLL_WALL;
    editorCollisionTable[2].type = EDITOR_COLL_CIRCLE;
    editorCollisionTable[2].radius = BLOCK_SIZE * 0.3f;
    editorCollisionTable[5].type = EDITOR_COLL_STAIRS;
    editorCollisionTable[6].type = EDITOR_COLL_BOX;
    editorCollisionTable[6].width = BLOCK_SIZE * 0.35f;
    editorCollisionTable[6].depth = BLOCK_SIZE * 0.35f;

    ifstream ifs("collision_config.csv");
    if (!ifs)
        return;

    string line;
    while (getline(ifs, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        vector<string> cols = SplitCollisionCSV(line);
        if (cols.size() < 2 || cols[0] == "id")
            continue;

        int id = stoi(cols[0]);
        if (id < 0 || id >= MODEL_MAX)
            continue;

        editorCollisionTable[id].type = ParseEditorCollisionType(cols[1]);
        editorCollisionTable[id].radius = cols.size() >= 3 ? stof(cols[2]) : 0.0f;
        editorCollisionTable[id].width = cols.size() >= 4 ? stof(cols[3]) : 0.0f;
        editorCollisionTable[id].depth = cols.size() >= 5 ? stof(cols[4]) : 0.0f;
    }
}

static void LoadEditorModelConfig()
{
    // 2026-05-20: 今後モデルを増やしやすくするため、エディターのモデル一覧をCSVから読み込む。
    for (int tab = 0; tab < TAB_MAX; tab++)
    {
        tabModelCount[tab] = 0;
        for (int i = 0; i < 16; i++)
            tabModelList[tab][i] = 0;
    }
    ResetEditorPlacementModelMaps();

    ifstream ifs("model_config.csv");
    if (!ifs)
    {
        printfDx("model_config.csv load failed\n");
        return;
    }

    string line;
    while (getline(ifs, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        vector<string> cols = SplitCollisionCSV(line);
        if (cols.size() < 3 || cols[0] == "id")
            continue;

        int id = stoi(cols[0]);
        int tab = ParseModelTab(cols[1]);
        if (id < 0 || id >= MODEL_MAX || tab < 0)
            continue;

        string modelPath = cols[2];
        string palettePath = cols.size() >= 4 ? cols[3] : "-";
        float scaleX = HasCsvValue(cols, 4) ? stof(cols[4]) : 1.0f;
        float scaleY = HasCsvValue(cols, 5) ? stof(cols[5]) : scaleX;
        float scaleZ = HasCsvValue(cols, 6) ? stof(cols[6]) : scaleX;
        int placeId = HasCsvValue(cols, 7) ? stoi(cols[7]) : id;

        if (tab == EVENT)
        {
            placeId = HasCsvValue(cols, 7) ? stoi(cols[7]) : id;
        }
        else
        {
            // 2026-07-21: EVENT以外の8/9/10列目は、モデル表示位置の微調整値として使う。
            modelOffsetX[id] = HasCsvValue(cols, 7) ? stof(cols[7]) : 0.0f;
            modelOffsetZ[id] = HasCsvValue(cols, 8) ? stof(cols[8]) : 0.0f;
            modelOffsetY[id] = HasCsvValue(cols, 9) ? stof(cols[9]) : 0.0f;
        }

        if (modelPath != "-" && modelHandles[id] == -1)
        {
            modelHandles[id] = MV1LoadModel(modelPath.c_str());
            if (modelHandles[id] != -1)
                MV1SetScale(modelHandles[id], VGet(scaleX, scaleY, scaleZ));
        }

        if (palettePath != "-" && paletteTex[id] == -1)
            paletteTex[id] = LoadGraph(palettePath.c_str());

        if (tab == EVENT && placeId >= 0 && placeId < MODEL_MAX)
        {
            editorEventIdFromModel[id] = placeId;
            editorEventModelFromEventId[placeId] = id;
        }
        else if (tab == ENEMY)
        {
            editorEnemyIdFromModel[id] = HasCsvValue(cols, 7) ? placeId : 0;
        }

        if (!IsEditorPlacementDisabled(tab, id))
            AddModelToEditorTab(tab, id);
    }
}

static void DrawCollisionEdgeLine(int x, int y, int z, int rot, int scale, int thickness, int color);

static bool IsEditorCornerWallPairId(int id)
{
    // 2026-07-22: モデル28はいったん外したため、角モデル29だけ直角に交わる壁2枚として当たり判定を扱う。
    return id == 29;
}

static int GetEditorCornerWallPairCollisionRot(int id, int rot)
{
    // 2026-07-22: モデル28はいったん外したため、角モデル29の見た目に合わせた当たり判定補正だけ残す。
    int baseRot = rot & 3;
    int collisionRot = (baseRot + 1) & 3;

    // 2026-07-21: モデル29の当たり判定全体が1方向ずれていたため、全回転をさらに-1方向へ調整する。
    collisionRot = (collisionRot + 3) & 3;

    // 2026-07-21: モデル29だけ回転が2個分ずれていたため、29の当たり判定だけ180度補正する。
    collisionRot = (collisionRot + 2) & 3;

    return collisionRot;
}

static bool IsEditorCornerWallPairEdge(int rot, int edge)
{
    // 2026-07-21: 元のGetCornerArcDataと同じ回転対応にし、0=下左/1=下右/2=上右/3=上左の2辺へ合わせる。
    int r = rot & 3;
    int e = edge & 3;
    switch (r)
    {
    case 0: return e == 2 || e == 3;
    case 1: return e == 2 || e == 1;
    case 2: return e == 0 || e == 1;
    default: return e == 0 || e == 3;
    }
}

static void DrawCornerWallPairCollision(int x, int y, int z, int rot, int color)
{
    // 2026-07-21: エディターのプレビューでも、モデル28はいったん外したため、モデル29が壁2枚分の判定だと分かるよう2辺を表示する。
    for (int edge = 0; edge < 4; edge++)
    {
        if (IsEditorCornerWallPairEdge(rot, edge))
            DrawCollisionEdgeLine(x, y, z, edge, 100, 100, color);
    }
}
static void DrawCollisionEdgeLine(int x, int y, int z, int rot, int scale, int thickness, int color)
{
    float left = x * BLOCK_SIZE;
    float right = (x + 1) * BLOCK_SIZE;
    float front = z * BLOCK_SIZE;
    float back = (z + 1) * BLOCK_SIZE;
    float yy = y * BLOCK_SIZE + 18.0f;
    float cx = CellToWorldCenter(x);
    float cz = CellToWorldCenter(z);
    float lenRate = scale / 100.0f;

    VECTOR a = VGet(cx, yy, cz);
    VECTOR b = VGet(cx, yy, cz);
    switch (rot & 3)
    {
    case 0:
        a = VGet(cx - BLOCK_SIZE * 0.5f * lenRate, yy, front);
        b = VGet(cx + BLOCK_SIZE * 0.5f * lenRate, yy, front);
        break;
    case 1:
        a = VGet(right, yy, cz - BLOCK_SIZE * 0.5f * lenRate);
        b = VGet(right, yy, cz + BLOCK_SIZE * 0.5f * lenRate);
        break;
    case 2:
        a = VGet(cx - BLOCK_SIZE * 0.5f * lenRate, yy, back);
        b = VGet(cx + BLOCK_SIZE * 0.5f * lenRate, yy, back);
        break;
    case 3:
        a = VGet(left, yy, cz - BLOCK_SIZE * 0.5f * lenRate);
        b = VGet(left, yy, cz + BLOCK_SIZE * 0.5f * lenRate);
        break;
    }
    DrawLine3D(a, b, color);
}

static void DrawCollisionBox(int x, int y, int z, float width, float depth, int color)
{
    float cx = CellToWorldCenter(x);
    float cz = CellToWorldCenter(z);
    float yy = y * BLOCK_SIZE + 20.0f;
    float hw = width * 0.5f;
    float hd = depth * 0.5f;
    DrawLine3D(VGet(cx - hw, yy, cz - hd), VGet(cx + hw, yy, cz - hd), color);
    DrawLine3D(VGet(cx + hw, yy, cz - hd), VGet(cx + hw, yy, cz + hd), color);
    DrawLine3D(VGet(cx + hw, yy, cz + hd), VGet(cx - hw, yy, cz + hd), color);
    DrawLine3D(VGet(cx - hw, yy, cz + hd), VGet(cx - hw, yy, cz - hd), color);
}

static void DrawCollisionCircle(int x, int y, int z, float radius, int color)
{
    VECTOR center = VGet(CellToWorldCenter(x), y * BLOCK_SIZE + 20.0f, CellToWorldCenter(z));
    DrawSphere3D(center, max(8.0f, radius * 0.08f), 8, color, GetColor(255, 255, 255), TRUE);
}

static void DrawCollisionArc(int x, int y, int z, int rot, float radius, float thickness, int color)
{
    float cx = CellToWorldCenter(x);
    float cz = CellToWorldCenter(z);
    float yy = y * BLOCK_SIZE + 24.0f;
    DrawLine3D(VGet(cx - radius * 0.5f, yy, cz), VGet(cx + radius * 0.5f, yy, cz), color);
    DrawLine3D(VGet(cx, yy, cz - radius * 0.5f), VGet(cx, yy, cz + radius * 0.5f), color);
}

static void DrawCollisionCornerLine(int x, int y, int z, int rot, int scale, int thickness, int offset, int color)
{
    float left = x * BLOCK_SIZE;
    float right = (x + 1) * BLOCK_SIZE;
    float front = z * BLOCK_SIZE;
    float back = (z + 1) * BLOCK_SIZE;
    float yy = y * BLOCK_SIZE + 24.0f;
    DrawLine3D(VGet(left, yy, front), VGet(right, yy, back), color);
}

static void DrawHighPlatformCollision(int x, int y, int z, int color)
{
    // 2026-07-14: 0.5床の上面だけを確認できるよう、簡易の四角線で表示する。
    DrawCollisionBox(x, y, z, BLOCK_SIZE * 0.75f, BLOCK_SIZE * 0.75f, color);
}

static void DrawDefaultObjectCollision(int tab, int id, int x, int y, int z, int rot, int color)
{
    // 2026-05-11: オブジェクトが最初から持っている既定当たり判定を、配置後すぐ見えるよう追加。
    if (id < 0 || id >= MODEL_MAX)
        return;

    if (tab == CORNER && IsEditorCornerWallPairId(id))
    {
        // 2026-07-21: モデル28/29は見た目に合わせ、角を構成する壁2枚分の当たり判定として表示する。
        DrawCornerWallPairCollision(x, y, z, GetEditorCornerWallPairCollisionRot(id, rot), color);
        return;
    }

    if (tab == WALL || editorCollisionTable[id].type == EDITOR_COLL_WALL)
    {
        DrawCollisionEdgeLine(x, y, z, rot, 100, 100, color);
        return;
    }

    if (tab == DECO && id == 7)
    {
        DrawHighPlatformCollision(x, y, z, color);
        return;
    }

    if (tab == CORNER && editorCollisionTable[id].type == EDITOR_COLL_NONE)
    {
        DrawCollisionCornerLine(
            x,
            y,
            z,
            rot,
            CollisionCornerScaleMap[y][z][x],
            CollisionCornerThicknessMap[y][z][x],
            CollisionCornerOffsetMap[y][z][x],
            color);
        return;
    }

    if (editorCollisionTable[id].type == EDITOR_COLL_CIRCLE)
        DrawCollisionCircle(x, y, z, editorCollisionTable[id].radius, color);
    else if (editorCollisionTable[id].type == EDITOR_COLL_BOX)
        DrawCollisionBox(x, y, z, editorCollisionTable[id].width, editorCollisionTable[id].depth, color);
    else if (editorCollisionTable[id].type == EDITOR_COLL_ARC)
        DrawCollisionArc(x, y, z, rot, editorCollisionTable[id].radius + 40.0f, max(10.0f, editorCollisionTable[id].width), color);
}

#pragma endregion




