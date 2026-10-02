static bool HasMiniMapContent(int y, int z, int x)
{
    if (!IsMapPosValid(y, z, x))
        return false;

    return
        GameFloorMap[y][z][x] >= 0 ||
        GameWallMapA[y][z][x] >= 0 ||
        GameWallMapB[y][z][x] >= 0 ||
        GameCornerMap[y][z][x] >= 0 ||
        GameDecoMap[y][z][x] >= 0 ||
        GameCollisionMap[y][z][x] >= 0;
}
static bool MiniMapHasLocalWallEdge(int y, int z, int x, int edge)
{
    // 2026-06-28: HasWallEdgeは移動判定用に隣セルの壁も拾うため、ミニマップでは自セルに置かれた壁だけ描く。
    if (!IsMapPosValid(y, z, x))
        return false;

    int rot = edge & 3;
    if (GameWallMapA[y][z][x] >= 0 && GameWallRotA[y][z][x] == rot)
        return true;

    if (GameWallMapB[y][z][x] >= 0 && GameWallRotB[y][z][x] == rot)
        return true;

    if (GameCollisionEdgeMap[y][z][x] >= 0 &&
        (GameCollisionEdgeMap[y][z][x] & GetCollisionEdgeBit(rot)) != 0)
        return true;

    return false;
}

static int CountMiniMapVisitedCells(int y)
{
    int count = 0;

    for (int z = 0; z < BLOCK_NUM_Z; z++)
    {
        for (int x = 0; x < BLOCK_NUM_X; x++)
        {
            if (GameMiniMapVisited[y][z][x])
                count++;
        }
    }

    return count;
}

static void RevealMiniMapAroundPlayer(VECTOR playerPos, int layer)
{
    int playerCellX = WorldToCell(playerPos.x);
    int playerCellZ = WorldToCell(playerPos.z);

    // 2026-06-28: 1マスだけだとミニマップの見える範囲が狭すぎるため、周囲1マスも探索済みにする。
    for (int dz = -1; dz <= 1; dz++)
    {
        for (int dx = -1; dx <= 1; dx++)
        {
            int x = playerCellX + dx;
            int z = playerCellZ + dz;
            if (IsMapPosValid(layer, z, x))
                GameMiniMapVisited[layer][z][x] = true;
        }
    }
}

// 2026-05-21: ミニマップのズレ修正マップを反転させた後のプレイヤーの位置修正のため追加
static int MiniMapToScreenInt(float value)
{
    // 2026-05-25: ミニマップの向きはそのままに、int切り捨てで出ていた半ピクセル程度のズレを丸めて微調整する。
    return (int)(value);
}

static int MiniMapPosX(int mapX, float cellSize, float cellX)
{
    return mapX + MiniMapToScreenInt(cellX * cellSize);
}

static int MiniMapPosY(int mapY, float cellSize, float cellZ)
{
    return mapY + MiniMapToScreenInt((BLOCK_NUM_Z - cellZ) * cellSize);
}
void DrawMiniMap(VECTOR playerPos)
{
    const int mapSize = 180;
    const int margin = 20;
    const int layerPanelW = 44;
    const int mapX = SCREEN_W - mapSize - margin;
    const int mapY = margin;
    const int layerX = mapX - layerPanelW - 8;
    const float cellSize = mapSize / (float)BLOCK_NUM_X;

    // 2026-06-25: ジャンプの高さでミニマップ階層が切り替わらないよう、現在ロード中階層を使う。
    int layer = GetGameLoadedLayer();
    RevealMiniMapAroundPlayer(playerPos, layer);

    SetUseZBuffer3D(FALSE);
    SetWriteZBuffer3D(FALSE);

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 180);
    DrawBox(layerX - 6, mapY - 6, mapX + mapSize + 6, mapY + mapSize + 28, GetColor(0, 0, 0), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // 2026-05-18: 壁と床の差が弱く見にくかったため、暗い床と明るい壁色で見分けやすくする。
    DrawBox(mapX, mapY, mapX + mapSize, mapY + mapSize, GetColor(8, 12, 18), TRUE);

    for (int z = 0; z < BLOCK_NUM_Z; z++)
    {
        for (int x = 0; x < BLOCK_NUM_X; x++)
        {
            if (!GameMiniMapVisited[layer][z][x])
                continue;

            // int left = mapX + (int)(x * cellSize);
            // int top = mapY + (int)(z * cellSize);
            // int right = mapX + (int)((x + 1) * cellSize);
            // int bottom = mapY + (int)((z + 1) * cellSize);
            int left = MiniMapPosX(mapX, cellSize, (float)x);
            int right = MiniMapPosX(mapX, cellSize, (float)(x + 1));

            int top = MiniMapPosY(mapY, cellSize, (float)(z + 1));
            int bottom = MiniMapPosY(mapY, cellSize, (float)z);
            bool walkable = GameFloorMap[layer][z][x] >= 0;
            if (walkable)
                DrawBox(left + 1, top + 1, right - 1, bottom - 1, GetColor(42, 58, 66), TRUE);
            if (HasClimbHintAtCell(layer, z, x))
            {
                // 2026-07-16: 0.5床から上層床へ跳べる場所を、ミニマップでは階段マーク風に表示する。
                int cx = (left + right) / 2;
                int cy = (top + bottom) / 2;
                DrawBox(cx - 3, cy - 3, cx + 3, cy + 3, GetColor(255, 220, 90), TRUE);

                if (IsClimbHintDirection(layer, z, x, 0)) DrawLine(cx, cy, cx, top + 3, GetColor(255, 220, 90), 2);
                if (IsClimbHintDirection(layer, z, x, 1)) DrawLine(cx, cy, right - 3, cy, GetColor(255, 220, 90), 2);
                if (IsClimbHintDirection(layer, z, x, 2)) DrawLine(cx, cy, cx, bottom - 3, GetColor(255, 220, 90), 2);
                if (IsClimbHintDirection(layer, z, x, 3)) DrawLine(cx, cy, left + 3, cy, GetColor(255, 220, 90), 2);
            }

            if (HasClimbDownHintAtCell(layer, z, x))
            {
                // 2026-07-16: 上層床側にも、下の0.5床へ降りられる場所をミニマップへ表示する。
                int cx = (left + right) / 2;
                int cy = (top + bottom) / 2;
                DrawCircle(cx, cy, 4, GetColor(255, 150, 70), TRUE);

                if (IsClimbDownHintDirection(layer, z, x, 0)) DrawLine(cx, cy, cx, top + 3, GetColor(255, 150, 70), 2);
                if (IsClimbDownHintDirection(layer, z, x, 1)) DrawLine(cx, cy, right - 3, cy, GetColor(255, 150, 70), 2);
                if (IsClimbDownHintDirection(layer, z, x, 2)) DrawLine(cx, cy, cx, bottom - 3, GetColor(255, 150, 70), 2);
                if (IsClimbDownHintDirection(layer, z, x, 3)) DrawLine(cx, cy, left + 3, cy, GetColor(255, 150, 70), 2);
            }


            if (GameCollisionMap[layer][z][x] >= 0)
                DrawBox(left + 2, top + 2, right - 2, bottom - 2, GetColor(120, 55, 70), TRUE);

            // 2026-06-28: ミニマップはZ方向を上下反転しているため、上/下の壁線も画面上では入れ替えて描く。
            if (MiniMapHasLocalWallEdge(layer, z, x, 0))
                DrawLine(left, bottom, right, bottom, GetColor(120, 235, 255), 3);

            if (MiniMapHasLocalWallEdge(layer, z, x, 1))
                DrawLine(right, top, right, bottom, GetColor(120, 235, 255), 3);

            if (MiniMapHasLocalWallEdge(layer, z, x, 2))
                DrawLine(left, top, right, top, GetColor(120, 235, 255), 3);

            if (MiniMapHasLocalWallEdge(layer, z, x, 3))
                DrawLine(left, top, left, bottom, GetColor(120, 235, 255), 3);

            if (GameCornerMap[layer][z][x] >= 0)
            {
                if (GameCornerRot[layer][z][x] % 2 == 0)
                    DrawLine(left + 2, bottom - 2, right - 2, top + 2, GetColor(120, 235, 255), 3);
                else
                    DrawLine(right - 2, bottom - 2, left + 2, top + 2, GetColor(120, 235, 255), 3);
            }
        }
    }

    for (int y = 0; y < BLOCK_NUM_Y; y++)
    {
        int top = mapY + y * 22;
        int bottom = top + 18;
        int color = y == layer ? GetColor(255, 210, 80) : GetColor(70, 78, 88);
        int visited = CountMiniMapVisitedCells(y);

        DrawBox(layerX, top, layerX + layerPanelW, bottom, color, TRUE);
        DrawFormatString(layerX + 6, top + 2, GetColor(0, 0, 0), "%d", y);

        if (visited > 0)
        {
            int gaugeW = visited * (layerPanelW - 18) / (BLOCK_NUM_X * BLOCK_NUM_Z);
            if (gaugeW < 2) gaugeW = 2;
            DrawBox(layerX + 18, top + 6, layerX + 18 + gaugeW, top + 12, GetColor(90, 190, 255), TRUE);
        }
    }

    auto DrawCellMarker = [&](int y, int z, int x, int color)
        {
            if (y != layer || !IsMapPosValid(y, z, x) || !GameMiniMapVisited[y][z][x])
                return;
            int cx = MiniMapPosX(mapX, cellSize, x + 0.5f);
            int cy = MiniMapPosY(mapY, cellSize, z + 0.5f);

            DrawCircle(cx, cy, 5, color, TRUE);
        };

    DrawCellMarker(gameStartY, gameStartZ, gameStartX, GetColor(80, 180, 255));
    DrawCellMarker(gameGoalY, gameGoalZ, gameGoalX, GetColor(255, 220, 70));

    for (int i = 0; i < e.GetEnemyCount(); i++)
    {
        VECTOR enemyPos;
        int enemyLayer = 0;
        if (!e.GetEnemyPosition(i, enemyPos, enemyLayer))
            continue;

        int enemyX = WorldToCell(enemyPos.x);
        int enemyZ = WorldToCell(enemyPos.z);
        DrawCellMarker(enemyLayer, enemyZ, enemyX, GetColor(255, 70, 95));
    }

    float playerCellX = playerPos.x / BLOCK_SIZE;
    float playerCellZ = playerPos.z / BLOCK_SIZE;

    int playerX = MiniMapPosX(mapX, cellSize, playerCellX);
    int playerY = MiniMapPosY(mapY, cellSize, playerCellZ);

    //int playerX = mapX + (int)((playerPos.x / BLOCK_SIZE) * cellSize);
    //int playerY = mapY + (int)((playerPos.z / BLOCK_SIZE) * cellSize);
    DrawCircle(playerX, playerY, 6, GetColor(80, 230, 255), TRUE);
    DrawCircle(playerX, playerY, 6, GetColor(255, 255, 255), FALSE);

    DrawFormatString(mapX, mapY + mapSize + 8, GetColor(255, 255, 255), "LAYER %d", layer);

    SetUseZBuffer3D(TRUE);
    SetWriteZBuffer3D(TRUE);
}





