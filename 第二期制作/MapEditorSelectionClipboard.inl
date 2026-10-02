#pragma region ===== コピー処理 =====

static void ClearCopyBuffer()
{
    copySizeX = 0;
    copySizeZ = 0;
    hasCopyData = false;

    for (int z = 0; z < COPY_MAX_Z; z++)
    {
        for (int x = 0; x < COPY_MAX_X; x++)
        {
            CopyFloorMap[z][x] = -1;
            CopyWallMapA[z][x] = -1;
            CopyWallMapB[z][x] = -1;
            CopyCornerMap[z][x] = -1;
            CopyDecoMap[z][x] = -1;
            // 2026-06-11: コピー初期化時に前回の敵配置を残さないため追加。
            // CopyEnemyMap[z][x] = -1;
            CopyEventMap[z][x] = -1;
            CopyEventRot[z][x] = 0;
            // 2026-07-21: コピー初期化時にEVENT回転もリセットし、扉の向きが前回コピーから残らないようにする。
            // 2026-05-11: コピー用バッファ初期化時に当たり判定の古い値を残さないため追加。
            CopyCollisionMap[z][x] = -1;
            // 2026-05-20: コピー初期化でもBoxCollider風の調整値を標準値へ戻す。
            CopyCollisionBoxOffsetXMap[z][x] = 0;
            CopyCollisionBoxOffsetZMap[z][x] = 0;
            CopyCollisionBoxSizeXMap[z][x] = (int)BLOCK_SIZE;
            CopyCollisionBoxSizeZMap[z][x] = (int)BLOCK_SIZE;
            // 2026-05-11: コピー用バッファ初期化時に辺当たり判定の古い値を残さないため追加。
            CopyCollisionEdgeMap[z][x] = -1;
            // 2026-05-13: コピー用バッファ初期化時にコーナー当たり調整値の古い値を残さないため追加。
            CopyCollisionCornerScaleMap[z][x] = 100;
            CopyCollisionCornerThicknessMap[z][x] = 100;
            CopyCollisionCornerOffsetMap[z][x] = 0;
            for (int edge = 0; edge < 4; edge++)
            {
                CopyCollisionEdgeScaleMap[z][x][edge] = 100;
                CopyCollisionEdgeThicknessMap[z][x][edge] = 100;
            }

            CopyFloorRot[z][x] = 0;
            CopyWallRotA[z][x] = 0;
            CopyWallRotB[z][x] = 0;
            CopyCornerRot[z][x] = 0;
            CopyDecoRot[z][x] = 0;
        }
    }
}

static void CopySelection()
{
    bool hasRangeSelection = HasValidSelection();
    bool hasMultiSelection = multiSelectCount > 0;
    if (!hasRangeSelection && !hasMultiSelection)
        return;

    int sourceLayer = selectLayer;
    int minX = 0;
    int maxX = 0;
    int minZ = 0;
    int maxZ = 0;

    if (hasRangeSelection)
    {
        minX = min(selectStartX, selectEndX);
        maxX = max(selectStartX, selectEndX);
        minZ = min(selectStartZ, selectEndZ);
        maxZ = max(selectStartZ, selectEndZ);
    }
    else
    {
        bool found = false;
        for (int layer = 0; layer < BLOCK_NUM_Y && !found; layer++)
        {
            for (int z = 0; z < BLOCK_NUM_Z && !found; z++)
            {
                for (int x = 0; x < BLOCK_NUM_X; x++)
                {
                    if (multiSelectMap[layer][z][x])
                    {
                        sourceLayer = layer;
                        minX = maxX = x;
                        minZ = maxZ = z;
                        found = true;
                        break;
                    }
                }
            }
        }

        if (!found)
            return;

        for (int z = 0; z < BLOCK_NUM_Z; z++)
        {
            for (int x = 0; x < BLOCK_NUM_X; x++)
            {
                if (!multiSelectMap[sourceLayer][z][x])
                    continue;

                if (x < minX) minX = x;
                if (x > maxX) maxX = x;
                if (z < minZ) minZ = z;
                if (z > maxZ) maxZ = z;
            }
        }
    }

    ClearCopyBuffer();

    copySizeX = maxX - minX + 1;
    copySizeZ = maxZ - minZ + 1;

    for (int z = 0; z < copySizeZ; z++)
    {
        for (int x = 0; x < copySizeX; x++)
        {
            int srcX = minX + x;
            int srcZ = minZ + z;

            if (!hasRangeSelection && !multiSelectMap[sourceLayer][srcZ][srcX])
                continue;

            CopyFloorMap[z][x] = FloorMap[sourceLayer][srcZ][srcX];
            CopyFloorRot[z][x] = FloorRot[sourceLayer][srcZ][srcX];

            CopyWallMapA[z][x] = WallMapA[sourceLayer][srcZ][srcX];
            CopyWallMapB[z][x] = WallMapB[sourceLayer][srcZ][srcX];
            CopyWallRotA[z][x] = WallRotA[sourceLayer][srcZ][srcX];
            CopyWallRotB[z][x] = WallRotB[sourceLayer][srcZ][srcX];

            CopyCornerMap[z][x] = CornerMap[sourceLayer][srcZ][srcX];
            CopyCornerRot[z][x] = CornerRot[sourceLayer][srcZ][srcX];

            CopyDecoMap[z][x] = DecoMap[sourceLayer][srcZ][srcX];
            CopyDecoRot[z][x] = DecoRot[sourceLayer][srcZ][srcX];

            CopyEventMap[z][x] = EventMap[sourceLayer][srcZ][srcX];
            CopyEventRot[z][x] = EventRot[sourceLayer][srcZ][srcX];

            CopyCollisionMap[z][x] = CollisionMap[sourceLayer][srcZ][srcX];
            CopyCollisionBoxOffsetXMap[z][x] = CollisionBoxOffsetXMap[sourceLayer][srcZ][srcX];
            CopyCollisionBoxOffsetZMap[z][x] = CollisionBoxOffsetZMap[sourceLayer][srcZ][srcX];
            CopyCollisionBoxSizeXMap[z][x] = CollisionBoxSizeXMap[sourceLayer][srcZ][srcX];
            CopyCollisionBoxSizeZMap[z][x] = CollisionBoxSizeZMap[sourceLayer][srcZ][srcX];
            CopyCollisionEdgeMap[z][x] = CollisionEdgeMap[sourceLayer][srcZ][srcX];
            CopyCollisionCornerScaleMap[z][x] = CollisionCornerScaleMap[sourceLayer][srcZ][srcX];
            CopyCollisionCornerThicknessMap[z][x] = CollisionCornerThicknessMap[sourceLayer][srcZ][srcX];
            CopyCollisionCornerOffsetMap[z][x] = CollisionCornerOffsetMap[sourceLayer][srcZ][srcX];
            for (int edge = 0; edge < 4; edge++)
            {
                CopyCollisionEdgeScaleMap[z][x][edge] = CollisionEdgeScaleMap[sourceLayer][srcZ][srcX][edge];
                CopyCollisionEdgeThicknessMap[z][x][edge] = CollisionEdgeThicknessMap[sourceLayer][srcZ][srcX][edge];
            }
        }
    }

    hasCopyData = true;
}
static void PasteSelection(int dstLayer, int dstX, int dstZ)
{
    if (!hasCopyData)
        return;

    for (int z = 0; z < copySizeZ; z++)
    {
        for (int x = 0; x < copySizeX; x++)
        {
            int targetX = dstX + x;
            int targetZ = dstZ + z;

            if (!IsMapPosValid(dstLayer, targetZ, targetX))
                continue;

            FloorMap[dstLayer][targetZ][targetX] = CopyFloorMap[z][x];
            FloorRot[dstLayer][targetZ][targetX] = CopyFloorRot[z][x];

            WallMapA[dstLayer][targetZ][targetX] = CopyWallMapA[z][x];
            WallMapB[dstLayer][targetZ][targetX] = CopyWallMapB[z][x];
            WallRotA[dstLayer][targetZ][targetX] = CopyWallRotA[z][x];
            WallRotB[dstLayer][targetZ][targetX] = CopyWallRotB[z][x];

            CornerMap[dstLayer][targetZ][targetX] = CopyCornerMap[z][x];
            CornerRot[dstLayer][targetZ][targetX] = CopyCornerRot[z][x];

            DecoMap[dstLayer][targetZ][targetX] = CopyDecoMap[z][x];
            DecoRot[dstLayer][targetZ][targetX] = CopyDecoRot[z][x];

            // 2026-06-11: 範囲貼り付けでも敵配置をEnemyMapへ復元するため追加。
            // EnemyMap[dstLayer][targetZ][targetX] = CopyEnemyMap[z][x];
            PlaceEventWithRot(dstLayer, targetZ, targetX, CopyEventMap[z][x], CopyEventRot[z][x]);
            // 2026-07-21: 範囲貼り付けでもEVENTの向きを復元するため追加。
            // 2026-05-11: 貼り付け時に手動当たり判定も同じ位置へ復元するため追加。
            CollisionMap[dstLayer][targetZ][targetX] = CopyCollisionMap[z][x];
            // 2026-05-20: 範囲貼り付けでBoxCollider風の調整値も復元するため追加。
            CollisionBoxOffsetXMap[dstLayer][targetZ][targetX] = CopyCollisionBoxOffsetXMap[z][x];
            CollisionBoxOffsetZMap[dstLayer][targetZ][targetX] = CopyCollisionBoxOffsetZMap[z][x];
            CollisionBoxSizeXMap[dstLayer][targetZ][targetX] = CopyCollisionBoxSizeXMap[z][x];
            CollisionBoxSizeZMap[dstLayer][targetZ][targetX] = CopyCollisionBoxSizeZMap[z][x];
            // 2026-05-11: 貼り付け時に辺当たり判定も同じ位置へ復元するため追加。
            CollisionEdgeMap[dstLayer][targetZ][targetX] = CopyCollisionEdgeMap[z][x];
            // 2026-05-13: 貼り付け時にコーナー当たり調整値も同じ位置へ復元するため追加。
            CollisionCornerScaleMap[dstLayer][targetZ][targetX] = CopyCollisionCornerScaleMap[z][x];
            CollisionCornerThicknessMap[dstLayer][targetZ][targetX] = CopyCollisionCornerThicknessMap[z][x];
            CollisionCornerOffsetMap[dstLayer][targetZ][targetX] = CopyCollisionCornerOffsetMap[z][x];
            for (int edge = 0; edge < 4; edge++)
            {
                CollisionEdgeScaleMap[dstLayer][targetZ][targetX][edge] = CopyCollisionEdgeScaleMap[z][x][edge];
                CollisionEdgeThicknessMap[dstLayer][targetZ][targetX][edge] = CopyCollisionEdgeThicknessMap[z][x][edge];
            }
        }
    }
}

// 2026-06-24: コピー元配列を壊さず、描画時と配置時にだけ座標変換するため追加。
// 何度回転しても元データが劣化せず、キャンセルも状態を捨てるだけで済む。
static void GetRotatedCopyPosition(int srcX, int srcZ, int rotation, int& rotatedX, int& rotatedZ)
{
    switch (rotation & 3)
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
    default:
        rotatedX = srcX;
        rotatedZ = srcZ;
        break;
    }
}

static void GetRotatedCopySize(int rotation, int& sizeX, int& sizeZ)
{
    if (rotation & 1)
    {
        sizeX = copySizeZ;
        sizeZ = copySizeX;
    }
    else
    {
        sizeX = copySizeX;
        sizeZ = copySizeZ;
    }
}

static void RotateCopyBoxValues(
    int offsetX, int offsetZ, int sizeX, int sizeZ, int rotation,
    int& rotatedOffsetX, int& rotatedOffsetZ, int& rotatedSizeX, int& rotatedSizeZ)
{
    rotatedOffsetX = offsetX;
    rotatedOffsetZ = offsetZ;
    rotatedSizeX = sizeX;
    rotatedSizeZ = sizeZ;

    for (int turn = 0; turn < (rotation & 3); turn++)
    {
        int oldOffsetX = rotatedOffsetX;
        rotatedOffsetX = -rotatedOffsetZ;
        rotatedOffsetZ = oldOffsetX;

        int oldSizeX = rotatedSizeX;
        rotatedSizeX = rotatedSizeZ;
        rotatedSizeZ = oldSizeX;
    }
}

// 2026-06-24: モデルだけでなく手動コライダーの向きも含め、コピー範囲全体を
// 回転した状態で配置する。範囲外へ出たセルは従来どおり安全に読み飛ばす。
static void PasteSelectionRotated(int dstLayer, int dstX, int dstZ, int rotation)
{
    if (!hasCopyData)
        return;

    rotation &= 3;

    for (int srcZ = 0; srcZ < copySizeZ; srcZ++)
    {
        for (int srcX = 0; srcX < copySizeX; srcX++)
        {
            int rotatedX = 0;
            int rotatedZ = 0;
            GetRotatedCopyPosition(srcX, srcZ, rotation, rotatedX, rotatedZ);

            int targetX = dstX + rotatedX;
            int targetZ = dstZ + rotatedZ;

            if (!IsMapPosValid(dstLayer, targetZ, targetX))
                continue;

            FloorMap[dstLayer][targetZ][targetX] = CopyFloorMap[srcZ][srcX];
            FloorRot[dstLayer][targetZ][targetX] = (CopyFloorRot[srcZ][srcX] + rotation) & 3;

            WallMapA[dstLayer][targetZ][targetX] = CopyWallMapA[srcZ][srcX];
            WallMapB[dstLayer][targetZ][targetX] = CopyWallMapB[srcZ][srcX];
            WallRotA[dstLayer][targetZ][targetX] = (CopyWallRotA[srcZ][srcX] + rotation) & 3;
            WallRotB[dstLayer][targetZ][targetX] = (CopyWallRotB[srcZ][srcX] + rotation) & 3;

            CornerMap[dstLayer][targetZ][targetX] = CopyCornerMap[srcZ][srcX];
            CornerRot[dstLayer][targetZ][targetX] = (CopyCornerRot[srcZ][srcX] + rotation) & 3;

            DecoMap[dstLayer][targetZ][targetX] = CopyDecoMap[srcZ][srcX];
            DecoRot[dstLayer][targetZ][targetX] = (CopyDecoRot[srcZ][srcX] + rotation) & 3;

            PlaceEventWithRot(dstLayer, targetZ, targetX, CopyEventMap[srcZ][srcX], (CopyEventRot[srcZ][srcX] + rotation) & 3);
            // 2026-07-21: 回転貼り付け時、扉などEVENTモデルの向きもコピー全体の回転に合わせる。
            CollisionMap[dstLayer][targetZ][targetX] = CopyCollisionMap[srcZ][srcX];

            RotateCopyBoxValues(
                CopyCollisionBoxOffsetXMap[srcZ][srcX],
                CopyCollisionBoxOffsetZMap[srcZ][srcX],
                CopyCollisionBoxSizeXMap[srcZ][srcX],
                CopyCollisionBoxSizeZMap[srcZ][srcX],
                rotation,
                CollisionBoxOffsetXMap[dstLayer][targetZ][targetX],
                CollisionBoxOffsetZMap[dstLayer][targetZ][targetX],
                CollisionBoxSizeXMap[dstLayer][targetZ][targetX],
                CollisionBoxSizeZMap[dstLayer][targetZ][targetX]);

            // 2026-06-25: CopyCollisionEdgeMap が -1 の時にビット判定すると全辺ON扱いになり、
            // コピー配置後の当たり判定が暴れるため、辺なしは -1 のまま復元する。
            if (CopyCollisionEdgeMap[srcZ][srcX] < 0)
            {
                CollisionEdgeMap[dstLayer][targetZ][targetX] = -1;
                for (int edge = 0; edge < 4; edge++)
                {
                    CollisionEdgeScaleMap[dstLayer][targetZ][targetX][edge] = 100;
                    CollisionEdgeThicknessMap[dstLayer][targetZ][targetX][edge] = 100;
                }
            }
            else
            {
                CollisionEdgeMap[dstLayer][targetZ][targetX] = 0;
                for (int edge = 0; edge < 4; edge++)
                {
                    int rotatedEdge = (edge + rotation) & 3;
                    if (CopyCollisionEdgeMap[srcZ][srcX] & GetCollisionEdgeBit(edge))
                        CollisionEdgeMap[dstLayer][targetZ][targetX] |= GetCollisionEdgeBit(rotatedEdge);

                    CollisionEdgeScaleMap[dstLayer][targetZ][targetX][rotatedEdge] =
                        CopyCollisionEdgeScaleMap[srcZ][srcX][edge];
                    CollisionEdgeThicknessMap[dstLayer][targetZ][targetX][rotatedEdge] =
                        CopyCollisionEdgeThicknessMap[srcZ][srcX][edge];
                }

                if (CollisionEdgeMap[dstLayer][targetZ][targetX] == 0)
                    CollisionEdgeMap[dstLayer][targetZ][targetX] = -1;
            }

            CollisionCornerScaleMap[dstLayer][targetZ][targetX] =
                CopyCollisionCornerScaleMap[srcZ][srcX];
            CollisionCornerThicknessMap[dstLayer][targetZ][targetX] =
                CopyCollisionCornerThicknessMap[srcZ][srcX];
            CollisionCornerOffsetMap[dstLayer][targetZ][targetX] =
                CopyCollisionCornerOffsetMap[srcZ][srcX];
        }
    }
}

#pragma endregion


