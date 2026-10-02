#pragma region ===== Editor 3D Rendering =====

static bool ShouldDrawEditorLayer(int layer)
{
    return !showCurrentLayerOnly || layer == currentLayer;
}

static void DrawEditorModelWithEvenLight(int modelHandle)
{
    // 2026-07-21: Restore comment. DxLib in this project has no MV1SetMaterialUseLighting, so disable lighting only while drawing editor models.
    int oldLighting = GetLightEnable();
    SetUseLighting(FALSE);
    SetLightEnable(FALSE);
    MV1DrawModel(modelHandle);
    SetLightEnable(oldLighting);
    SetUseLighting(TRUE);
}

static void ApplyEditorLockedDoorDrawOffset(VECTOR& pos, int rot)
{
    // 2026-07-21: Restore comment. Locked door uses the same cell-edge placement as walls, so editor and play mode line up.
    const float halfBlock = BLOCK_SIZE * 0.5f;
    switch (rot & 3)
    {
    case 0: pos.z -= halfBlock; break;
    case 1: pos.x += halfBlock; break;
    case 2: pos.z += halfBlock; break;
    case 3: pos.x -= halfBlock; break;
    }
}

static bool IsEditorCustomColor(int colorIndex)
{
    // 2026-07-21: 0?5の既存プリセットと区別するため、自由色は上位bitを付けて保存する。
    return (colorIndex & 0x01000000) != 0;
}

static COLOR_F GetEditorPlacedColorF(int colorIndex)
{
    // 2026-07-21: 色変更ボタンで保存した色番号を、DxLibのマテリアル色へ変換する。
    if (IsEditorCustomColor(colorIndex))
    {
        int rgb = colorIndex & 0x00ffffff;
        return GetColorF(((rgb >> 16) & 255) / 255.0f, ((rgb >> 8) & 255) / 255.0f, (rgb & 255) / 255.0f, 1.0f);
    }

    switch (colorIndex % 6)
    {
    case 1: return GetColorF(1.0f, 0.35f, 0.35f, 1.0f);
    case 2: return GetColorF(0.35f, 0.55f, 1.0f, 1.0f);
    case 3: return GetColorF(0.35f, 1.0f, 0.50f, 1.0f);
    case 4: return GetColorF(1.0f, 0.86f, 0.30f, 1.0f);
    case 5: return GetColorF(0.72f, 0.42f, 1.0f, 1.0f);
    default: return GetColorF(1.0f, 1.0f, 1.0f, 1.0f);
    }
}

static int GetEditorPlacedDrawColor(int colorIndex)
{
    // 2026-07-21: モデルが無い簡易床表示にも、色変更ボタンの色を反映する。
    if (IsEditorCustomColor(colorIndex))
    {
        int rgb = colorIndex & 0x00ffffff;
        return GetColor((rgb >> 16) & 255, (rgb >> 8) & 255, rgb & 255);
    }

    switch (colorIndex % 6)
    {
    case 1: return GetColor(255, 90, 90);
    case 2: return GetColor(90, 140, 255);
    case 3: return GetColor(90, 220, 120);
    case 4: return GetColor(255, 220, 70);
    case 5: return GetColor(190, 110, 255);
    default: return GetColor(82, 72, 58);
    }
}

static bool IsEditorLowPlatformDecoId(int id)
{
    // 2026-07-22: ローダー側のIsLowPlatformDecoIdと同じく、床レイヤー6を半ブロック足場として描く。
    return id == 6;
}

static void GetEditorClimbHintDirOffset(int dir, int& dx, int& dz)
{
    // 2026-07-22: ローダー側のはしご表示と同じ方向対応で、L接続の向きをエディターでも判断する。
    dx = 0;
    dz = 0;
    switch (dir & 3)
    {
    case 0: dz = -1; break;
    case 1: dx = 1; break;
    case 2: dz = 1; break;
    case 3: dx = -1; break;
    }
}

static bool HasEditorWallEdgeForClimb(int layer, int z, int x, int dir)
{
    // 2026-07-22: はしごは壁で塞がれた方向には出さないよう、配置済み壁A/Bの辺と衝突編集の辺を確認する。
    int bit = GetCollisionEdgeBit(dir);
    if ((CollisionEdgeMap[layer][z][x] & bit) != 0)
        return true;
    if (WallMapA[layer][z][x] >= 0 && WallRotA[layer][z][x] == (dir & 3))
        return true;
    if (WallMapB[layer][z][x] >= 0 && WallRotB[layer][z][x] == (dir & 3))
        return true;
    return false;
}

static bool IsEditorClimbHintDirection(int y, int z, int x, int dir)
{
    // 2026-07-22: 段差6はLキーの手動ON/OFFを使わず、隣の上層床へ届く方向を自動接続としてエディターにはしご表示する。
    if (!IsMapPosValid(y, z, x) || !IsEditorLowPlatformDecoId(DecoMap[y][z][x]))
        return false;

    int dx = 0;
    int dz = 0;
    GetEditorClimbHintDirOffset(dir, dx, dz);

    int upperLayer = y + 1;
    int targetX = x + dx;
    int targetZ = z + dz;
    if (!IsMapPosValid(upperLayer, targetZ, targetX))
        return false;
    if (FloorMap[upperLayer][targetZ][targetX] < 0)
        return false;
    if (HasEditorWallEdgeForClimb(y, z, x, dir))
        return false;

    return true;
}

static void DrawEditorClimbHintEdge(int x, int y, int z, int dir, float opacity)
{
    // 2026-07-22: ローダー側で表示しているはしごID30を、エディターでも段差6の上層接続位置に表示する。
    const int ladderModelId = 30;
    if (ladderModelId < 0 || ladderModelId >= MODEL_MAX || modelHandles[ladderModelId] == -1)
        return;

    int dx = 0;
    int dz = 0;
    GetEditorClimbHintDirOffset(dir, dx, dz);

    VECTOR pos = VGet(
        CellToWorldCenter(x) + dx * BLOCK_SIZE * 0.42f,
        CellToWorldCenter(y),
        CellToWorldCenter(z) + dz * BLOCK_SIZE * 0.42f
    );
    pos = ApplyEditorModelConfigOffset(DECO, ladderModelId, pos);

    MV1SetPosition(modelHandles[ladderModelId], pos);
    MV1SetRotationXYZ(modelHandles[ladderModelId], VGet(0.0f, RotToRad(dir), 0.0f));
    MV1SetOpacityRate(modelHandles[ladderModelId], opacity);
    DrawEditorModelWithEvenLight(modelHandles[ladderModelId]);
    MV1SetOpacityRate(modelHandles[ladderModelId], 1.0f);
}

static void DrawEditorClimbHints(int x, int y, int z, float opacity)
{
    // 2026-07-22: 段差6から上層床につながる方向すべてに、ローダーと同じはしごを自動表示する。
    for (int dir = 0; dir < 4; dir++)
    {
        if (IsEditorClimbHintDirection(y, z, x, dir))
            DrawEditorClimbHintEdge(x, y, z, dir, opacity);
    }
}
static void DrawEditorModelWithPlacedColor(int modelHandle, int colorIndex)
{
    // 2026-07-21: 同じモデルを使い回しつつ、配置セルごとの色変更を描画時だけ反映する。
    // 2026-07-21: MaterialDifColorはテクスチャ付きモデルだと効きにくいため、モデル全体の色倍率で tint する。
    MV1SetDifColorScale(modelHandle, GetEditorPlacedColorF(colorIndex));
    MV1SetAmbColorScale(modelHandle, GetEditorPlacedColorF(colorIndex));
    DrawEditorModelWithEvenLight(modelHandle);
    MV1SetDifColorScale(modelHandle, GetColorF(1.0f, 1.0f, 1.0f, 1.0f));
    MV1SetAmbColorScale(modelHandle, GetColorF(1.0f, 1.0f, 1.0f, 1.0f));
}
static void DrawEditorFloorBlockBase(int x, int y, int z, float yOffset, float height, int fillColor, int edgeColor)
{
    float left = x * BLOCK_SIZE;
    float right = (x + 1) * BLOCK_SIZE;
    float front = z * BLOCK_SIZE;
    float back = (z + 1) * BLOCK_SIZE;
    float bottom = y * BLOCK_SIZE + yOffset;
    float top = bottom + height;

    DrawCube3D(
        VGet(left, bottom, front),
        VGet(right, top, back),
        fillColor,
        edgeColor,
        TRUE
    );
}

static void DrawEditorEventMarker(int layer, int z, int x, int eventId)
{
    // 2026-07-21: EVENTは登録モデルだけを見せるため、仮表示の色付き丸マーカーは描画しない。
}

static int GetEditorModelIdFromEnemyId(int enemyId)
{
    // 2026-07-21: ENEMYはCSV上のモデルIDと配置IDを分けているため、配置済み表示では配置IDからモデルIDへ戻す。
    for (int i = 0; i < tabModelCount[ENEMY]; i++)
    {
        int modelId = tabModelList[ENEMY][i];
        if (modelId >= 0 && modelId < MODEL_MAX && GetEditorEnemyIdFromModel(modelId) == enemyId)
            return modelId;
    }
    return -1;
}

static void DrawEditorEnemyMarker(int layer, int z, int x, int enemyId)
{
    int modelId = GetEditorModelIdFromEnemyId(enemyId);
    if (modelId < 0 || modelId >= MODEL_MAX || modelHandles[modelId] == -1)
        return;

    // 2026-07-21: ENEMY配置時にも完成位置が分かるよう、仮の丸ではなく登録モデルそのものを表示する。
    VECTOR pos = GetModelDrawPosition(ENEMY, x, layer, z, 0);
    pos = ApplyEditorModelConfigOffset(ENEMY, modelId, pos);
    pos.y += 8.0f;
    MV1SetPosition(modelHandles[modelId], pos);
    MV1SetRotationXYZ(modelHandles[modelId], VGet(0.0f, RotToRad(0), 0.0f));
    DrawEditorModelWithEvenLight(modelHandles[modelId]);
}

static void DrawEditorPatrolPointMarker(int layer, int z, int x, int color)
{
    float cx = CellToWorldCenter(x);
    float cz = CellToWorldCenter(z);
    float y = layer * BLOCK_SIZE + 72.0f;
    float half = BLOCK_SIZE * 0.11f;
    // 2026-07-22: 巡回ポイントは実体モデルではないため、小さい箱マーカーで配置位置だけ見せる。
    DrawCube3D(
        VGet(cx - half, y, cz - half),
        VGet(cx + half, y + 34.0f, cz + half),
        color,
        GetColor(30, 80, 110),
        TRUE
    );
}

static void DrawEditorAllEnemyPatrolMarkers()
{
    for (int layer = 0; layer < BLOCK_NUM_Y; layer++)
    {
        if (!ShouldDrawEditorLayer(layer))
            continue;

        for (int z = 0; z < BLOCK_NUM_Z; z++)
        {
            for (int x = 0; x < BLOCK_NUM_X; x++)
            {
                if (EnemyMap[layer][z][x] < 0)
                    continue;

                int count = EnemyPatrolCountMap[layer][z][x];
                if (count <= 0)
                    continue;

                bool selected = layer == selectedPatrolEnemyLayer && z == selectedPatrolEnemyZ && x == selectedPatrolEnemyX;
                int lineColor = selected ? GetColor(80, 220, 255) : GetColor(70, 150, 190);
                int pointColor = selected ? GetColor(80, 220, 255) : GetColor(80, 170, 210);
                float lineY = layer * BLOCK_SIZE + 112.0f;
                float lastX = CellToWorldCenter(x);
                float lastZ = CellToWorldCenter(z);

                for (int i = 0; i < count; i++)
                {
                    int px = EnemyPatrolXMap[layer][z][x][i];
                    int pz = EnemyPatrolZMap[layer][z][x][i];
                    if (!IsMapPosValid(layer, pz, px))
                        continue;

                    float toX = CellToWorldCenter(px);
                    float toZ = CellToWorldCenter(pz);
                    DrawLine3D(VGet(lastX, lineY, lastZ), VGet(toX, lineY, toZ), lineColor);
                    DrawEditorPatrolPointMarker(layer, pz, px, pointColor);
                    lastX = toX;
                    lastZ = toZ;
                }
            }
        }
    }
}
static void DrawEditorEnemyPatrolPlacementPreview()
{
    if (!enemyPatrolEditMode || currentTab != ENEMY)
        return;
    if (!IsMapPosValid(selectedPatrolEnemyLayer, selectedPatrolEnemyZ, selectedPatrolEnemyX))
        return;
    if (EnemyMap[selectedPatrolEnemyLayer][selectedPatrolEnemyZ][selectedPatrolEnemyX] < 0)
        return;
    if (!ShouldDrawEditorLayer(selectedPatrolEnemyLayer))
        return;

    int lineColor = GetColor(80, 220, 255);
    int pointColor = GetColor(80, 220, 255);
    int previewColor = GetColor(255, 240, 90);
    float y = selectedPatrolEnemyLayer * BLOCK_SIZE + 70.0f;
    float fromX = CellToWorldCenter(selectedPatrolEnemyX);
    float fromZ = CellToWorldCenter(selectedPatrolEnemyZ);
    float lastX = fromX;
    float lastZ = fromZ;

    // 2026-07-22: 巡回線は敵から全点へ放射状に出さず、置いた順番の最後の点から次へ伸びるようにする。
    int count = EnemyPatrolCountMap[selectedPatrolEnemyLayer][selectedPatrolEnemyZ][selectedPatrolEnemyX];
    for (int i = 0; i < count; i++)
    {
        int px = EnemyPatrolXMap[selectedPatrolEnemyLayer][selectedPatrolEnemyZ][selectedPatrolEnemyX][i];
        int pz = EnemyPatrolZMap[selectedPatrolEnemyLayer][selectedPatrolEnemyZ][selectedPatrolEnemyX][i];
        if (!IsMapPosValid(selectedPatrolEnemyLayer, pz, px))
            continue;

        float toX = CellToWorldCenter(px);
        float toZ = CellToWorldCenter(pz);
        DrawLine3D(VGet(lastX, y, lastZ), VGet(toX, y, toZ), lineColor);
        DrawEditorPatrolPointMarker(selectedPatrolEnemyLayer, pz, px, pointColor);
        lastX = toX;
        lastZ = toZ;
    }

    if (IsHoverValid() && hoverX >= 0 && hoverZ >= 0 && currentLayer == selectedPatrolEnemyLayer)
    {
        float toX = CellToWorldCenter(hoverX);
        float toZ = CellToWorldCenter(hoverZ);
        DrawLine3D(VGet(lastX, y + 12.0f, lastZ), VGet(toX, y + 12.0f, toZ), previewColor);
        DrawEditorPatrolPointMarker(currentLayer, hoverZ, hoverX, previewColor);
    }
}
static void DrawEditorEventConfiguredModel(int layer, int z, int x, int eventId, int rot, float opacity)
{
    int modelId = GetEditorModelIdFromEventId(eventId);
    if (modelId < 0 || modelId >= MODEL_MAX || modelHandles[modelId] == -1)
        return;

    VECTOR pos = GetModelDrawPosition(EVENT, x, layer, z, 0);
    pos.y += 8.0f;
    if (eventId == 5)
    {
        // 2026-07-21: Restore comment. Match game-side locked door offset so editor placement preview lines up with play mode.
        ApplyEditorLockedDoorDrawOffset(pos, rot);
    }

    // 2026-07-21: Restore comment. EVENT models use saved EventRot so doors keep the direction chosen with R.
    int drawRot = GetEditorModelDrawRot(EVENT, modelId, rot);
    MV1SetPosition(modelHandles[modelId], pos);
    MV1SetRotationXYZ(modelHandles[modelId], VGet(0.0f, RotToRad(drawRot), 0.0f));
    MV1SetOpacityRate(modelHandles[modelId], opacity);
    DrawEditorModelWithEvenLight(modelHandles[modelId]);
    MV1SetOpacityRate(modelHandles[modelId], 1.0f);
}

static void DrawCopyPreviewModel(int tab, int id, int x, int y, int z, int rot)
{
    if (id < 0 || id >= MODEL_MAX || modelHandles[id] == -1)
        return;


    VECTOR pos = GetModelDrawPosition(tab, x, y, z, rot);
    pos = ApplyEditorModelConfigOffset(tab, id, pos);
    pos = ApplyEditorWallModelAlignmentOffset(tab, id, rot, pos);
    pos.y += 12.0f;

    int drawRot = GetEditorModelDrawRot(tab, id, rot);
    MV1SetPosition(modelHandles[id], pos);
    MV1SetRotationXYZ(modelHandles[id], VGet(0.0f, RotToRad(drawRot), 0.0f));
    MV1SetOpacityRate(modelHandles[id], 0.45f);
    DrawEditorModelWithEvenLight(modelHandles[id]);
    if (tab == DECO && IsEditorLowPlatformDecoId(id))
    {
        // 2026-07-22: コピー配置プレビューではモデル描画後に地面から半ブロック分の箱を重ね、段差6の足場面を見せる。
        DrawEditorFloorBlockBase(x, y, z, 0.0f, BLOCK_SIZE * 0.5f, GetEditorPlacedDrawColor(editorColorIndex), GetColor(42, 35, 27));
    }
    MV1SetOpacityRate(modelHandles[id], 1.0f);
}

static void DrawCopyPreview()
{
    if (!pasteMode || !hasCopyData || !IsHoverValid())
        return;

    int rotation = copyRotation & 3;
    int previewSizeX = (rotation & 1) ? copySizeZ : copySizeX;
    int previewSizeZ = (rotation & 1) ? copySizeX : copySizeZ;

    for (int srcZ = 0; srcZ < copySizeZ; srcZ++)
    {
        for (int srcX = 0; srcX < copySizeX; srcX++)
        {
            int rotatedX = srcX;
            int rotatedZ = srcZ;
            switch (rotation)
            {
            case 1:
                rotatedX = copySizeZ - 1 - srcZ;
                rotatedZ = srcX;
                break;
            case 2:
                rotatedX = copySizeX - 1 - srcX;
                rotatedZ = copySizeZ - 1 - srcZ;
                break;
            case 3:
                rotatedX = srcZ;
                rotatedZ = copySizeX - 1 - srcX;
                break;
            }

            int previewX = hoverX + rotatedX;
            int previewZ = hoverZ + rotatedZ;
            if (!IsMapPosValid(currentLayer, previewZ, previewX))
                continue;

            DrawCopyPreviewModel(FLOOR, CopyFloorMap[srcZ][srcX], previewX, currentLayer, previewZ, (CopyFloorRot[srcZ][srcX] + rotation) & 3);
            DrawCopyPreviewModel(WALL, CopyWallMapA[srcZ][srcX], previewX, currentLayer, previewZ, (CopyWallRotA[srcZ][srcX] + rotation) & 3);
            DrawCopyPreviewModel(WALL, CopyWallMapB[srcZ][srcX], previewX, currentLayer, previewZ, (CopyWallRotB[srcZ][srcX] + rotation) & 3);
            DrawCopyPreviewModel(CORNER, CopyCornerMap[srcZ][srcX], previewX, currentLayer, previewZ, (CopyCornerRot[srcZ][srcX] + rotation) & 3);
            DrawCopyPreviewModel(DECO, CopyDecoMap[srcZ][srcX], previewX, currentLayer, previewZ, (CopyDecoRot[srcZ][srcX] + rotation) & 3);

            if (CopyEventMap[srcZ][srcX] >= 0)
            {
                // 2026-07-21: Restore comment. Copy preview rotates EVENT doors together with the copied room.
                DrawEditorEventConfiguredModel(currentLayer, previewZ, previewX, CopyEventMap[srcZ][srcX], (CopyEventRot[srcZ][srcX] + rotation) & 3, 0.45f);
                DrawEditorEventMarker(currentLayer, previewZ, previewX, CopyEventMap[srcZ][srcX]);
            }
        }
    }

    float left = hoverX * BLOCK_SIZE;
    float top = hoverZ * BLOCK_SIZE;
    float right = (hoverX + previewSizeX) * BLOCK_SIZE;
    float bottom = (hoverZ + previewSizeZ) * BLOCK_SIZE;
    float y = currentLayer * BLOCK_SIZE + 24.0f;
    int color = GetColor(255, 255, 0);
    DrawLine3D(VGet(left, y, top), VGet(right, y, top), color);
    DrawLine3D(VGet(right, y, top), VGet(right, y, bottom), color);
    DrawLine3D(VGet(right, y, bottom), VGet(left, y, bottom), color);
    DrawLine3D(VGet(left, y, bottom), VGet(left, y, top), color);
}

static void DrawEditorGrid()
{
    if (!showGrid)
        return;

    int color = GetColor(70, 70, 70);
    float gridY = currentLayer * BLOCK_SIZE;
    for (int z = 0; z <= BLOCK_NUM_Z; z++)
        DrawLine3D(VGet(0.0f, gridY, z * BLOCK_SIZE), VGet(BLOCK_NUM_X * BLOCK_SIZE, gridY, z * BLOCK_SIZE), color);
    for (int x = 0; x <= BLOCK_NUM_X; x++)
        DrawLine3D(VGet(x * BLOCK_SIZE, gridY, 0.0f), VGet(x * BLOCK_SIZE, gridY, BLOCK_NUM_Z * BLOCK_SIZE), color);
}

static void DrawEditorSelectionRect()
{
    if (!HasValidSelection())
        return;

    int minX = min(selectStartX, selectEndX);
    int maxX = max(selectStartX, selectEndX);
    int minZ = min(selectStartZ, selectEndZ);
    int maxZ = max(selectStartZ, selectEndZ);
    float x1 = minX * BLOCK_SIZE;
    float x2 = (maxX + 1) * BLOCK_SIZE;
    float z1 = minZ * BLOCK_SIZE;
    float z2 = (maxZ + 1) * BLOCK_SIZE;
    float y = selectLayer * BLOCK_SIZE + 15.0f;
    int color = GetColor(0, 255, 255);
    DrawLine3D(VGet(x1, y, z1), VGet(x2, y, z1), color);
    DrawLine3D(VGet(x2, y, z1), VGet(x2, y, z2), color);
    DrawLine3D(VGet(x2, y, z2), VGet(x1, y, z2), color);
    DrawLine3D(VGet(x1, y, z2), VGet(x1, y, z1), color);
}

static void DrawEditorCellSelectionOutline(int layer, int z, int x, int color)
{
    float x1 = x * BLOCK_SIZE;
    float x2 = (x + 1) * BLOCK_SIZE;
    float z1 = z * BLOCK_SIZE;
    float z2 = (z + 1) * BLOCK_SIZE;
    float y = layer * BLOCK_SIZE + 18.0f;

    DrawLine3D(VGet(x1, y, z1), VGet(x2, y, z1), color);
    DrawLine3D(VGet(x2, y, z1), VGet(x2, y, z2), color);
    DrawLine3D(VGet(x2, y, z2), VGet(x1, y, z2), color);
    DrawLine3D(VGet(x1, y, z2), VGet(x1, y, z1), color);
}

static void DrawEditorMultiSelectionRects()
{
    if (multiSelectCount <= 0)
        return;

    int color = GetColor(0, 255, 255);
    int accentColor = GetColor(255, 255, 0);
    for (int layer = 0; layer < BLOCK_NUM_Y; layer++)
    {
        if (!ShouldDrawEditorLayer(layer))
            continue;

        for (int z = 0; z < BLOCK_NUM_Z; z++)
        {
            for (int x = 0; x < BLOCK_NUM_X; x++)
            {
                if (!multiSelectMap[layer][z][x])
                    continue;

                DrawEditorCellSelectionOutline(layer, z, x, color);
                float cx = CellToWorldCenter(x);
                float cz = CellToWorldCenter(z);
                float y = layer * BLOCK_SIZE + 20.0f;
                DrawLine3D(VGet(cx - 10.0f, y, cz), VGet(cx + 10.0f, y, cz), accentColor);
                DrawLine3D(VGet(cx, y, cz - 10.0f), VGet(cx, y, cz + 10.0f), accentColor);
            }
        }
    }
}

static void DrawEditorPlacedModels()
{
    for (int y = 0; y < BLOCK_NUM_Y; y++)
    {
        if (!ShouldDrawEditorLayer(y))
            continue;

        float opacity = (y == currentLayer) ? 1.0f : 0.22f;
        // 2026-07-23: 半透明の別階層がZを書き込むと、現在階層へ置く配置プレビューを隠してしまうため別階層だけZ書き込みを止める。
        SetWriteZBuffer3D(y == currentLayer ? TRUE : FALSE);
        for (int z = 0; z < BLOCK_NUM_Z; z++)
        {
            for (int x = 0; x < BLOCK_NUM_X; x++)
            {
                int id = FloorMap[y][z][x];
                if (id >= 0 && id < MODEL_MAX && modelHandles[id] != -1)
                {
                    VECTOR pos = ApplyEditorModelConfigOffset(FLOOR, id, GetModelDrawPosition(FLOOR, x, y, z, FloorRot[y][z][x]));
                    MV1SetPosition(modelHandles[id], pos);
                    MV1SetRotationXYZ(modelHandles[id], VGet(0.0f, RotToRad(FloorRot[y][z][x]), 0.0f));
                    MV1SetOpacityRate(modelHandles[id], opacity);
                    DrawEditorModelWithPlacedColor(modelHandles[id], FloorColorMap[y][z][x]);
                    MV1SetOpacityRate(modelHandles[id], 1.0f);
                }
                else if (FloorMap[y][z][x] >= 0)
                {
                    DrawEditorFloorBlockBase(x, y, z, 0.0f, BLOCK_SIZE * 0.08f, GetEditorPlacedDrawColor(FloorColorMap[y][z][x]), GetColor(38, 32, 26));
                }

                id = WallMapA[y][z][x];
                if (id >= 0 && id < MODEL_MAX && modelHandles[id] != -1)
                {
                    VECTOR pos = ApplyEditorModelConfigOffset(WALL, id, GetModelDrawPosition(WALL, x, y, z, WallRotA[y][z][x]));
                    pos = ApplyEditorWallModelAlignmentOffset(WALL, id, WallRotA[y][z][x], pos);
                    MV1SetPosition(modelHandles[id], pos);
                    MV1SetRotationXYZ(modelHandles[id], VGet(0.0f, RotToRad(GetEditorModelDrawRot(WALL, id, WallRotA[y][z][x])), 0.0f));
                    MV1SetOpacityRate(modelHandles[id], opacity);
                    DrawEditorModelWithPlacedColor(modelHandles[id], WallColorMapA[y][z][x]);
                    MV1SetOpacityRate(modelHandles[id], 1.0f);
                }

                id = WallMapB[y][z][x];
                if (id >= 0 && id < MODEL_MAX && modelHandles[id] != -1)
                {
                    VECTOR pos = ApplyEditorModelConfigOffset(WALL, id, GetModelDrawPosition(WALL, x, y, z, WallRotB[y][z][x]));
                    pos = ApplyEditorWallModelAlignmentOffset(WALL, id, WallRotB[y][z][x], pos);
                    MV1SetPosition(modelHandles[id], pos);
                    MV1SetRotationXYZ(modelHandles[id], VGet(0.0f, RotToRad(GetEditorModelDrawRot(WALL, id, WallRotB[y][z][x])), 0.0f));
                    MV1SetOpacityRate(modelHandles[id], opacity);
                    DrawEditorModelWithPlacedColor(modelHandles[id], WallColorMapB[y][z][x]);
                    MV1SetOpacityRate(modelHandles[id], 1.0f);
                }

                id = CornerMap[y][z][x];
                if (id >= 0 && id < MODEL_MAX && modelHandles[id] != -1)
                {
                    VECTOR pos = ApplyEditorModelConfigOffset(CORNER, id, GetModelDrawPosition(CORNER, x, y, z, CornerRot[y][z][x]));
                    MV1SetPosition(modelHandles[id], pos);
                    // 2026-07-21: モデル28はいったん外したため、角モデル29の補正回転だけを配置済み表示にも反映してズレを防ぐ。
                    MV1SetRotationXYZ(modelHandles[id], VGet(0.0f, RotToRad(GetEditorModelDrawRot(CORNER, id, CornerRot[y][z][x])), 0.0f));
                    MV1SetOpacityRate(modelHandles[id], opacity);
                    DrawEditorModelWithPlacedColor(modelHandles[id], CornerColorMap[y][z][x]);
                    MV1SetOpacityRate(modelHandles[id], 1.0f);
                }

                id = DecoMap[y][z][x];
                if (id >= 0 && id < MODEL_MAX && modelHandles[id] != -1)
                {
                    int drawRot = (IsEditorStairsId(id) ? GetEditorStairsDrawRot(DecoRot[y][z][x]) : DecoRot[y][z][x]);
                    VECTOR pos = ApplyEditorModelConfigOffset(DECO, id, GetModelDrawPosition(DECO, x, y, z, DecoRot[y][z][x]));
                    MV1SetPosition(modelHandles[id], pos);
                    MV1SetRotationXYZ(modelHandles[id], VGet(0.0f, RotToRad(drawRot), 0.0f));
                    MV1SetOpacityRate(modelHandles[id], opacity);
                    DrawEditorModelWithPlacedColor(modelHandles[id], DecoColorMap[y][z][x]);
                    if (IsEditorLowPlatformDecoId(id))
                    {
                        // 2026-07-22: 配置済みの段差6はモデル描画後に地面から半ブロック分の箱を重ね、画像の見た目を前面に出す。
                        DrawEditorFloorBlockBase(x, y, z, 0.0f, BLOCK_SIZE * 0.5f, GetEditorPlacedDrawColor(DecoColorMap[y][z][x]), GetColor(42, 35, 27));
                        DrawEditorClimbHints(x, y, z, opacity);
                    }
                    MV1SetOpacityRate(modelHandles[id], 1.0f);
                }

                if (EventMap[y][z][x] >= 0)
                    DrawEditorEventConfiguredModel(y, z, x, EventMap[y][z][x], EventRot[y][z][x], opacity);

                if (EnemyMap[y][z][x] >= 0)
                    DrawEditorEnemyMarker(y, z, x, EnemyMap[y][z][x]);
            }
        }
    }
    SetWriteZBuffer3D(TRUE);
}

static void DrawEditorMarkers()
{
    for (int y = 0; y < BLOCK_NUM_Y; y++)
    {
        if (!ShouldDrawEditorLayer(y))
            continue;
        for (int z = 0; z < BLOCK_NUM_Z; z++)
            for (int x = 0; x < BLOCK_NUM_X; x++)
                if (EventMap[y][z][x] >= 0)
                    DrawEditorEventMarker(y, z, x, EventMap[y][z][x]);
    }

    DrawEditorAllEnemyPatrolMarkers();
}

static void DrawEditorHoverPreview()
{
    if (!IsHoverValid() || pasteMode)
        return;

    int previewId = GetSelectedModel();
    if (previewId < 0 || previewId >= MODEL_MAX || modelHandles[previewId] == -1)
        return;

    if (currentTab == ENEMY && FloorMap[currentLayer][hoverZ][hoverX] < 0)
        return;

    int previewRot = GetEditorPlacementRot(currentTab, hoverZ, hoverX);
    VECTOR pos = GetModelDrawPosition(currentTab, hoverX, currentLayer, hoverZ, previewRot);
    pos = ApplyEditorModelConfigOffset(currentTab, previewId, pos);
    pos = ApplyEditorWallModelAlignmentOffset(currentTab, previewId, previewRot, pos);
    pos.y += 8.0f;
    if (currentTab == EVENT && GetEditorEventIdFromModel(previewId) == 5)
        ApplyEditorLockedDoorDrawOffset(pos, previewRot);

    int previewDrawRot = GetEditorModelDrawRot(currentTab, previewId, previewRot);
    MV1SetPosition(modelHandles[previewId], pos);
    MV1SetRotationXYZ(modelHandles[previewId], VGet(0.0f, RotToRad(previewDrawRot), 0.0f));
    MV1SetOpacityRate(modelHandles[previewId], 0.45f);
    DrawEditorModelWithPlacedColor(modelHandles[previewId], editorColorIndex);
    if (currentTab == DECO && IsEditorLowPlatformDecoId(previewId))
    {
        // 2026-07-22: 段差6の配置前プレビューはモデル描画後に地面から半ブロック分の箱を重ねて見せる。
        DrawEditorFloorBlockBase(hoverX, currentLayer, hoverZ, 0.0f, BLOCK_SIZE * 0.5f, GetEditorPlacedDrawColor(editorColorIndex), GetColor(42, 35, 27));
    }
    MV1SetOpacityRate(modelHandles[previewId], 1.0f);

    if (showCollisionDebug)
        DrawDefaultObjectCollision(currentTab, previewId, hoverX, currentLayer, hoverZ, previewRot, GetColor(255, 255, 0));
}

static void DrawEditorCollisionDebug()
{
    if (!showCollisionDebug && !collisionEditMode)
        return;

    int color = GetColor(255, 255, 0);
    for (int y = 0; y < BLOCK_NUM_Y; y++)
    {
        if (!ShouldDrawEditorLayer(y))
            continue;
        for (int z = 0; z < BLOCK_NUM_Z; z++)
        {
            for (int x = 0; x < BLOCK_NUM_X; x++)
            {
                if (CollisionMap[y][z][x] >= 0)
                    DrawCollisionBox(x, y, z, (float)CollisionBoxSizeXMap[y][z][x], (float)CollisionBoxSizeZMap[y][z][x], color);
                if (CollisionEdgeMap[y][z][x] >= 0)
                    for (int edge = 0; edge < 4; edge++)
                        if (CollisionEdgeMap[y][z][x] & (1 << edge))
                            DrawCollisionEdgeLine(x, y, z, edge, CollisionEdgeScaleMap[y][z][x][edge], CollisionEdgeThicknessMap[y][z][x][edge], color);
            }
        }
    }
}

void DrawEditor()
{
    // 2026-07-21: Restored after the rendering include was accidentally zeroed; keeps editor, preview, EVENT rotation, and UI visible.
    ClearDrawScreen();

    // 2026-07-21: 未選択時の左ドラッグ移動で更新した注視点を使い、見ている位置を保ったまま描画する。
    VECTOR target = VGet(camTargetX, currentLayer * BLOCK_SIZE, camTargetZ);
    float camX = target.x + sinf(camRotY) * cosf(camRotX) * camDist;
    float camZ = target.z + cosf(camRotY) * cosf(camRotX) * camDist;
    float camY = target.y + sinf(camRotX) * camDist + BLOCK_SIZE * 2.0f;
    SetCameraPositionAndTarget_UpVecY(VGet(camX, camY, camZ), target);

    DrawEditorGrid();
    DrawEditorPlacedModels();
    DrawCopyPreview();
    DrawEditorHoverPreview();
    DrawEditorEnemyPatrolPlacementPreview();
    DrawEditorCollisionDebug();
    DrawEditorMarkers();
    DrawEditorSelectionRect();
    DrawEditorMultiSelectionRects();

    int selectedPreviewModel = GetSelectedModel();
    // 2026-07-21: 素材未選択中は左ドラッグ移動用の状態なので、配置プレビュー当たり判定を出さない。
    if (IsHoverValid() && selectedPreviewModel >= 0)
        DrawDefaultObjectCollision(currentTab, selectedPreviewModel, hoverX, currentLayer, hoverZ, GetEditorPlacementRot(currentTab, hoverZ, hoverX), GetColor(255, 255, 0));
    DrawEditorUI();
    // 2026-07-22: 新規保存/名前変更/キット登録の入力欄がUIに隠れないよう、通常UIの後に重ねて描画する。
    DrawSaveNameInput();
    DrawKitNameInput();
    // 2026-07-22: 未保存のままマップ切替に入った時、入力だけ止まらず確認警告が必ず見えるよう最後に重ねて描画する。
    DrawLoadMapConfirm();
}

#pragma endregion
