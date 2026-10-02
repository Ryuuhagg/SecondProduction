static bool IsWalkableCell(int y, int z, int x)
{
    if (!IsMapPosValid(y, z, x))
        return false;

    if (GameFloorMap[y][z][x] >= 0)
        return true;

    // 2026-07-22: BFS/セル移動でも、プレイヤーが実際に乗れる0.5床と上面床を通行可能にする。
    if (IsLowPlatformDecoId(GameDecoMap[y][z][x]))
        return true;

    if (y > 0 && IsOverheadDecoId(GameDecoMap[y - 1][z][x]))
        return true;

    // 2026-07-16: 階段機能はいったん使わないため、階段だけでは歩けるセルにしない。
    return false;
}

static bool IsGameCornerWallPairId(int id)
{
    // 2026-07-21: モデル28/29は直角に交わる壁2枚の角モデルとして、壁辺判定へ変換する。
    return id == 28 || id == 29;
}

static int GetGameCornerWallPairCollisionRot(int id, int rot)
{
    // 2026-07-21: モデル28/29は描画時に個別回転補正しているため、当たり判定も見た目側の回転へ合わせる。
    int baseRot = rot & 3;
    int collisionRot = baseRot;
    if (id == 28)
        collisionRot = (baseRot + 3) & 3;
    else if (id == 29)
        collisionRot = (baseRot + 1) & 3;

    // 2026-07-21: 28/29の当たり判定全体が1方向ずれていたため、全回転をさらに-1方向へ調整する。
    if (id == 28 || id == 29)
        collisionRot = (collisionRot + 3) & 3;

    // 2026-07-21: モデル29だけ回転が2個分ずれていたため、29の当たり判定だけ180度補正する。
    if (id == 29)
        collisionRot = (collisionRot + 2) & 3;

    return collisionRot;
}

static bool IsGameCornerWallPairEdge(int rot, int edge)
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
bool HasWallEdge(int y, int z, int x, int edge)
{
    if (!IsMapPosValid(y, z, x))
        return true;

    auto HasLocalWall = [&](int cy, int cz, int cx, int rot)
        {
            if (!IsMapPosValid(cy, cz, cx))
                return false;

            if (GameWallMapA[cy][cz][cx] >= 0 &&
                GameWallRotA[cy][cz][cx] == rot)
            {
                return true;
            }

            if (GameWallMapB[cy][cz][cx] >= 0 &&
                GameWallRotB[cy][cz][cx] == rot)
            {
                return true;
            }

            if (HasManualCollisionEdge(cy, cz, cx, rot))
            {
                return true;
            }

            if (IsGameCornerWallPairId(GameCornerMap[cy][cz][cx]) &&
                IsGameCornerWallPairEdge(GetGameCornerWallPairCollisionRot(GameCornerMap[cy][cz][cx], GameCornerRot[cy][cz][cx]), rot))
            {
                // 2026-07-21: モデル28/29の角壁は、BFS/セル移動でも壁2辺として扱う。
                return true;
            }

            return false;
        };

    // 自セル
    if (HasLocalWall(y, z, x, edge))
        return true;

    // 隣セル側も確認
    switch (edge)
    {
    case 0: // 上
        return HasLocalWall(y, z - 1, x, 2);

    case 1: // 右
        return HasLocalWall(y, z, x + 1, 3);

    case 2: // 下
        return HasLocalWall(y, z + 1, x, 0);

    case 3: // 左
        return HasLocalWall(y, z, x - 1, 1);
    }

    return false;
}

bool CanMoveCellToCell(int y, int fromX, int fromZ, int toX, int toZ)
{
    if (!IsWalkableCell(y, toZ, toX))
        return false;

    int dx = toX - fromX;
    int dz = toZ - fromZ;

    if (dx == 0 && dz == -1)
    {
         if (HasWallEdge(y, fromZ, fromX, 0)) return false;
        //if (HasWallEdge(y, toZ, toX, 2)) return false;
    }
    else if (dx == 1 && dz == 0)
    {
        if (HasWallEdge(y, fromZ, fromX, 1)) return false;
        //if (HasWallEdge(y, toZ, toX, 3)) return false;
    }
    else if (dx == 0 && dz == 1)
    {
         if (HasWallEdge(y, fromZ, fromX, 2)) return false;
        // if (HasWallEdge(y, toZ, toX, 0)) return false;
    }
    else if (dx == -1 && dz == 0)
    {
         if (HasWallEdge(y, fromZ, fromX, 3)) return false;
        // if (HasWallEdge(y, toZ, toX, 1)) return false;
    }
    else
    {
        return false;
    }

    return true;
}

bool IsGoalCell(int y, int z, int x)
{
    return
        x == gameGoalX &&
        y == gameGoalY &&
        z == gameGoalZ;
}

#pragma endregion


#pragma region ===== ワールド判定 共通 =====

static bool IsStairsAtWorldInLayer(int y, float worldX, float worldZ)
{
    int x = WorldToCell(worldX);
    int z = WorldToCell(worldZ);

    if (!IsMapPosValid(y, z, x))
        return false;

    return IsStairsId(GameDecoMap[y][z][x]);
}

static bool CircleHitAABB(float cx, float cz, float radius, float left, float top, float right, float bottom)
{
    float nearestX = cx;
    float nearestZ = cz;

    if (nearestX < left) nearestX = left;
    if (nearestX > right) nearestX = right;
    if (nearestZ < top) nearestZ = top;
    if (nearestZ > bottom) nearestZ = bottom;

    float dx = cx - nearestX;
    float dz = cz - nearestZ;

    return (dx * dx + dz * dz) <= radius * radius;
}

static bool CircleHitCircle(float x1, float z1, float r1, float x2, float z2, float r2)
{
    float dx = x1 - x2;
    float dz = z1 - z2;
    float rr = r1 + r2;

    return dx * dx + dz * dz <= rr * rr;
}

static bool CircleHitBox(float cx, float cz, float radius, float bx, float bz, float halfW, float halfD)
{
    float left = bx - halfW;
    float right = bx + halfW;
    float top = bz - halfD;
    float bottom = bz + halfD;

    return CircleHitAABB(cx, cz, radius, left, top, right, bottom);
}
static void GetCornerArcData(int x, int z, int rot, float radius, float& cx, float& cz, float& start, float& end)
{
    // 2026-05-20: 画像のようなコーナー用90度円弧を、rotごとにセル角へ合わせて配置する。
    // 2026-05-20: モデルのカーブより左下へズレたため、内側補正を外してセル角を中心にする。
    float left = x * BLOCK_SIZE;
    float right = (x + 1) * BLOCK_SIZE;
    float top = z * BLOCK_SIZE;
    float bottom = (z + 1) * BLOCK_SIZE;

    // 2026-05-20: 回転2を正解にして、逆だった回転1と回転3だけを入れ替える。
    switch (rot % 4)
    {
    case 0: cx = left; cz = bottom; start = DX_PI_F * 1.5f; end = DX_PI_F * 2.0f; break;
    case 1: cx = right; cz = bottom; start = DX_PI_F; end = DX_PI_F * 1.5f; break;
    case 2: cx = right; cz = top; start = DX_PI_F * 0.5f; end = DX_PI_F; break;
    default: cx = left; cz = top; start = 0.0f; end = DX_PI_F * 0.5f; break;
    }
}

static bool IsAngleOnArc(float angle, float start, float end)
{
    // 2026-05-20: 円弧の角度範囲内か調べる。
    if (angle < 0.0f)
        angle += DX_PI_F * 2.0f;

    return angle >= start && angle <= end;
}

static bool CircleHitArc(float worldX, float worldZ, float playerRadius, int x, int z, int rot, float arcRadius, float thickness)
{
    // 2026-05-20: コーナー用90度円弧の帯とプレイヤー円の接触を判定する。
    float cx, cz, start, end;
    GetCornerArcData(x, z, rot, arcRadius, cx, cz, start, end);
    float dx = worldX - cx;
    float dz = worldZ - cz;
    float angle = atan2f(dz, dx);
    if (angle < 0.0f)
        angle += DX_PI_F * 2.0f;

    float inner = max(10.0f, arcRadius - thickness);
    float outer = arcRadius + thickness;
    float dist = sqrtf(dx * dx + dz * dz);

    if (IsAngleOnArc(angle, start, end) && dist >= inner - playerRadius && dist <= outer + playerRadius)
    {
        // 2026-05-20: 内側全体はふさがず、エディターで見えている厚い円弧の範囲だけを実判定にする。
        return true;
    }

    auto HitCapLine = [&](float a)
    {
        float ix = cx + cosf(a) * inner;
        float iz = cz + sinf(a) * inner;
        float ox = cx + cosf(a) * outer;
        float oz = cz + sinf(a) * outer;

        float vx = ox - ix;
        float vz = oz - iz;
        float wx = worldX - ix;
        float wz = worldZ - iz;
        float lenSq = vx * vx + vz * vz;
        float t = lenSq > 0.0f ? (wx * vx + wz * vz) / lenSq : 0.0f;
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;

        float nearestX = ix + vx * t;
        float nearestZ = iz + vz * t;
        float capDx = worldX - nearestX;
        float capDz = worldZ - nearestZ;
        return capDx * capDx + capDz * capDz <= playerRadius * playerRadius;
    };

    // 2026-05-20: 円弧の端から裏側へ入れる隙間だけを、見た目の端線と同じ位置で止める。
    return HitCapLine(start) || HitCapLine(end);
}
static bool IsPointInsideLowPlatformCell(int y, int z, int x, float worldX, float worldZ)
{
    if (!IsMapPosValid(y, z, x))
        return false;

    int decoId = GameDecoMap[y][z][x];
    if (!IsLowPlatformDecoId(decoId))
        return false;

    CollisionInfo& info = collisionTable[decoId];
    if (info.type != COLL_BOX)
        return false;

    float bx = CellToWorldCenter(x);
    float bz = CellToWorldCenter(z);

    return
        worldX >= bx - info.width &&
        worldX <= bx + info.width &&
        worldZ >= bz - info.depth &&
        worldZ <= bz + info.depth;
}

static const float FLOOR_BLOCK_VISIBLE_TOP_OFFSET = -2.0f;
static const float FLOOR_BLOCK_VISIBLE_THICKNESS = BLOCK_SIZE * 0.22f;
static const float LOW_PLATFORM_VISIBLE_THICKNESS = BLOCK_SIZE * 0.5f;

// 2026-07-15: 表示している床ブロックの上面・下面と当たり判定の高さをそろえる。
static float GetVisibleBlockTopY(int y, float topOffset)
{
    return y * BLOCK_SIZE + topOffset + FLOOR_BLOCK_VISIBLE_TOP_OFFSET;
}

static float GetVisibleBlockBottomY(int y, float topOffset, float height)
{
    return GetVisibleBlockTopY(y, topOffset) - height;
}

static bool IsCircleOnLowPlatformCell(int y, int z, int x, float worldX, float worldZ, float radius)
{
    if (!IsMapPosValid(y, z, x))
        return false;

    int decoId = GameDecoMap[y][z][x];
    if (!IsLowPlatformDecoId(decoId))
        return false;

    CollisionInfo& info = collisionTable[decoId];
    if (info.type != COLL_BOX)
        return false;

    float bx = CellToWorldCenter(x);
    float bz = CellToWorldCenter(z);

    return CircleHitBox(worldX, worldZ, radius, bx, bz, info.width, info.depth);
}

static bool FindLowPlatformAtWorld(int y, float worldX, float worldZ)
{
    int cellX = WorldToCell(worldX);
    int cellZ = WorldToCell(worldZ);

    // 2026-07-10: 半マス足場は通常床と同じく、現在座標のセルだけを判定する。
    return IsPointInsideLowPlatformCell(y, cellZ, cellX, worldX, worldZ);
}

static bool FindLowPlatformAtWorld(int y, float worldX, float worldZ, float radius)
{
    if (radius <= 0.0f)
        return FindLowPlatformAtWorld(y, worldX, worldZ);

    int cellX = WorldToCell(worldX);
    int cellZ = WorldToCell(worldZ);

    // 2026-07-13: プレイヤー中心だけだと端で沈むため、足元の円が重なる半マス足場を上面として扱う。
    for (int dz = -1; dz <= 1; dz++)
    {
        for (int dx = -1; dx <= 1; dx++)
        {
            if (IsCircleOnLowPlatformCell(y, cellZ + dz, cellX + dx, worldX, worldZ, radius))
                return true;
        }
    }

    return false;
}

static bool IsPointInsideCellRect(int z, int x, float worldX, float worldZ)
{
    const float edgeEpsilon = 1.0f;
    float left = x * BLOCK_SIZE;
    float right = (x + 1) * BLOCK_SIZE;
    float top = z * BLOCK_SIZE;
    float bottom = (z + 1) * BLOCK_SIZE;

    return
        worldX >= left - edgeEpsilon &&
        worldX <= right + edgeEpsilon &&
        worldZ >= top - edgeEpsilon &&
        worldZ <= bottom + edgeEpsilon;
}

static bool FindOverheadFloorAtWorld(int y, float worldX, float worldZ)
{
    if (y <= 0)
        return false;

    int cellX = WorldToCell(worldX);
    int cellZ = WorldToCell(worldZ);

    // 装飾7は1つ上の階の床になるため、境界の抜け防止で周囲セルも確認する。
    for (int dz = -1; dz <= 1; dz++)
    {
        for (int dx = -1; dx <= 1; dx++)
        {
            int x = cellX + dx;
            int z = cellZ + dz;

            if (!IsMapPosValid(y - 1, z, x))
                continue;

            if (IsOverheadDecoId(GameDecoMap[y - 1][z][x]) &&
                IsPointInsideCellRect(z, x, worldX, worldZ))
            {
                return true;
            }
        }
    }

    return false;
}

static bool IsWalkableAtWorld(int y, float worldX, float worldZ)
{
    int x = WorldToCell(worldX);
    int z = WorldToCell(worldZ);

    if (IsWalkableCell(y, z, x))
        return true;

    if (!IsMapPosValid(y, z, x))
        return false;

    // 1つ下の階に置いた装飾7は、今の階の床として扱う。
    // マス境界で床が抜けないよう、周囲セルも確認する。
    if (FindOverheadFloorAtWorld(y, worldX, worldZ))
        return true;

    // 装飾6の上面も周囲セルから探し、マス境界で足元が埋まらないようにする。
    return FindLowPlatformAtWorld(y, worldX, worldZ);
}
static bool HasWalkableSurfaceForCircle(int y, float worldX, float worldZ, float radius)
{
    // 2026-07-16: 階段機能はいったん使わないため、足場確認は床/0.5段差だけを見る。
    if (!IsWalkableAtWorld(y, worldX, worldZ)) return false;
    if (!IsWalkableAtWorld(y, worldX - radius, worldZ)) return false;
    if (!IsWalkableAtWorld(y, worldX + radius, worldZ)) return false;
    if (!IsWalkableAtWorld(y, worldX, worldZ - radius)) return false;
    if (!IsWalkableAtWorld(y, worldX, worldZ + radius)) return false;

    return true;
}
#pragma endregion


#pragma region ===== 壁 / 物 判定 =====

static bool IsSolidDecoId(int id)
{
    if (id < 0)
        return false;

    if (IsStairsId(id))
        return false;

    if (IsLowPlatformDecoId(id) || IsOverheadDecoId(id))
        return false;

    if (id >= MODEL_MAX)
        return false;

    return collisionTable[id].type != COLL_NONE;
}

static bool IsSolidCell(int y, int z, int x)
{
    if (!IsMapPosValid(y, z, x))
        return true;

    // 2026-05-11: エディターで手動設定したセル当たり判定を移動判定へ反映するため追加。
    if (GameCollisionMap[y][z][x] >= 0)
        return true;

    //if (GameCornerMap[y][z][x] >= 0)
    //    return true;

    if (IsSolidDecoId(GameDecoMap[y][z][x]))
        return true;

    return false;
}
bool IsCellBlocked(int y, int z, int x)
{
    // 2026-05-11: エディター/デバッグ側からセルの当たり判定状態を確認できるよう追加。
    return IsSolidCell(y, z, x);
}
static float PointLineDistance(
    float px,
    float pz,
    float x1,
    float z1,
    float x2,
    float z2)
{
    float vx = x2 - x1;
    float vz = z2 - z1;

    float wx = px - x1;
    float wz = pz - z1;

    float lenSq = vx * vx + vz * vz;

    float t = 0.0f;

    if (lenSq > 0.0f)
    {
        t = (wx * vx + wz * vz) / lenSq;
    }

    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    float nearestX = x1 + vx * t;
    float nearestZ = z1 + vz * t;

    float dx = px - nearestX;
    float dz = pz - nearestZ;

    return sqrtf(dx * dx + dz * dz);
}
static bool HitMapObjects(
    int y,
    float worldX,
    float worldZ,
    float radius,
    float currentY)
{
    int cellX = WorldToCell(worldX);
    int cellZ = WorldToCell(worldZ);

    for (int dz = -1; dz <= 1; dz++)
    {
        for (int dx = -1; dx <= 1; dx++)
        {
            int x = cellX + dx;
            int z = cellZ + dz;

            if (!IsMapPosValid(y, z, x))
                continue;

            // 2026-05-11: 手動当たり判定セルは、プレイヤー半径とセル矩形の接触で止めるため追加。
            if (GameCollisionMap[y][z][x] >= 0)
            {
                // 2026-05-20: セル全体ではなく、エディターで直接触って調整したBoxCollider風の範囲で止める。
                float boxCenterX = CellToWorldCenter(x) + GameCollisionBoxOffsetXMap[y][z][x];
                float boxCenterZ = CellToWorldCenter(z) + GameCollisionBoxOffsetZMap[y][z][x];
                float halfX = max(20.0f, GameCollisionBoxSizeXMap[y][z][x] * 0.5f);
                float halfZ = max(20.0f, GameCollisionBoxSizeZMap[y][z][x] * 0.5f);

                if (CircleHitAABB(worldX, worldZ, radius, boxCenterX - halfX, boxCenterZ - halfZ, boxCenterX + halfX, boxCenterZ + halfZ))
                    return true;
            }

            float left = x * BLOCK_SIZE;
            float right = (x + 1) * BLOCK_SIZE;
            float top = z * BLOCK_SIZE;
            float bottom = (z + 1) * BLOCK_SIZE;


            auto CheckWall = [&](int rot, int scale, int thicknessScale)
                {
                    float x1, z1;
                    float x2, z2;

                    // 各方向ごとの線位置
                    switch (rot % 4)
                    {
                    case 0: // 上壁
                        x1 = left + BLOCK_SIZE * 0.0f;
                        z1 = top;

                        x2 = right - BLOCK_SIZE * 0.0f;
                        z2 = top;
                        break;

                    case 1: // 右
                        x1 = right;
                        z1 = top + BLOCK_SIZE * 0.00f;

                        x2 = right;
                        z2 = bottom - BLOCK_SIZE * 0.00f;
                        break;
                    case 2: // 下壁
                        x1 = left + BLOCK_SIZE * 0.0f;
                        z1 = bottom;

                        x2 = right - BLOCK_SIZE * 0.0f;
                        z2 = bottom;
                        break;
                    case 3: // 左壁
                        x1 = left;
                        z1 = top + BLOCK_SIZE * 0.0f;

                        x2 = left;
                        z2 = bottom - BLOCK_SIZE * 0.0f;
                        break;


                    }

                    float centerX = (x1 + x2) * 0.5f;
                    float centerZ = (z1 + z2) * 0.5f;
                    float rate = scale / 100.0f;

                    x1 = centerX + (x1 - centerX) * rate;
                    z1 = centerZ + (z1 - centerZ) * rate;
                    x2 = centerX + (x2 - centerX) * rate;
                    z2 = centerZ + (z2 - centerZ) * rate;

                    float lineDX = x2 - x1;
                    float lineDZ = z2 - z1;
                    float lineLenSq = lineDX * lineDX + lineDZ * lineDZ;
                    float along = 0.0f;
                    if (lineLenSq > 0.0001f)
                    {
                        along = ((worldX - x1) * lineDX + (worldZ - z1) * lineDZ) / lineLenSq;
                    }

                    // 2026-07-15: ジャンプ中に壁端の角へめり込まないよう、端側の余白判定を広げる。
                    float lineLen = sqrtf(lineLenSq);
                    float endMarginRate = lineLen > 0.0001f ? (radius * 0.85f) / lineLen : 0.0f;
                    if (along < -endMarginRate || along > 1.0f + endMarginRate)
                        return false;

                    float dist = PointLineDistance(
                        worldX,
                        worldZ,
                        x1,
                        z1,
                        x2,
                        z2);

                    // 2026-06-29: 壁の線は見えている辺に置き、横へ広げず正面側の厚みで見た目の本体を埋める。
                    float signedDist = 0.0f;
                    switch (rot % 4)
                    {
                    case 0: signedDist = worldZ - z1; break;
                    case 1: signedDist = x1 - worldX; break;
                    case 2: signedDist = z1 - worldZ; break;
                    case 3: signedDist = worldX - x1; break;
                    }

                    float frontThickness = BLOCK_SIZE * 0.19f;
                    float backThickness = BLOCK_SIZE * 0.03f;
                    float rateThickness = thicknessScale / 100.0f;
                    frontThickness *= rateThickness;
                    backThickness *= rateThickness;
                    // 2026-07-15: 裏側まで正面厚みの箱で広がると、壁の後ろ判定が大きすぎるため線の表裏判定だけで止める。
                    if (signedDist >= 0.0f)
                        return dist <= radius + frontThickness;

                    float backRadius = radius * 0.75f;
                    // 2026-07-15: 壁裏の当たり判定が広く感じるため、裏側だけ半径と厚みを小さめにして見た目へ寄せる。
                    return dist <= backRadius + backThickness;
                };

            auto HasLocalPlacedWallEdge = [&](int rot)
                {
                    return
                        (GameWallMapA[y][z][x] >= 0 && GameWallRotA[y][z][x] == rot) ||
                        (GameWallMapB[y][z][x] >= 0 && GameWallRotB[y][z][x] == rot);
                };

            // 2026-06-29: 隣セルの壁辺をモデル判定へ取り込むと裏側が広くなりすぎるため、自セルの壁だけを見る。
            if (HasLocalPlacedWallEdge(0) && CheckWall(0, 100, 100))
            {
                return true;
            }

            if (HasLocalPlacedWallEdge(1) && CheckWall(1, 100, 100))
            {
                return true;
            }

            if (HasLocalPlacedWallEdge(2) && CheckWall(2, 100, 100))
            {
                return true;
            }

            if (HasLocalPlacedWallEdge(3) && CheckWall(3, 100, 100))
            {
                return true;
            }


            if (HasManualCollisionEdge(y, z, x, 0) && CheckWall(0, GetManualCollisionEdgeScale(y, z, x, 0), GetManualCollisionEdgeThickness(y, z, x, 0)))
            {
                return true;
            }

            if (HasManualCollisionEdge(y, z, x, 1) && CheckWall(1, GetManualCollisionEdgeScale(y, z, x, 1), GetManualCollisionEdgeThickness(y, z, x, 1)))
            {
                return true;
            }

            if (HasManualCollisionEdge(y, z, x, 2) && CheckWall(2, GetManualCollisionEdgeScale(y, z, x, 2), GetManualCollisionEdgeThickness(y, z, x, 2)))
            {
                return true;
            }

            if (HasManualCollisionEdge(y, z, x, 3) && CheckWall(3, GetManualCollisionEdgeScale(y, z, x, 3), GetManualCollisionEdgeThickness(y, z, x, 3)))
            {
                return true;
            }
            int cornerId = GameCornerMap[y][z][x];

            if (cornerId >= 0)
            {
                if (IsGameCornerWallPairId(cornerId))
                {
                    // 2026-07-21: モデル28/29は見た目通り、角を構成する壁2枚分の衝突判定にする。
                    for (int edge = 0; edge < 4; edge++)
                    {
                        if (IsGameCornerWallPairEdge(GetGameCornerWallPairCollisionRot(cornerId, GameCornerRot[y][z][x]), edge) && CheckWall(edge, 100, 100))
                            return true;
                    }
                    continue;
                }

                CollisionInfo& cornerInfo = collisionTable[cornerId];
                // 2026-05-20: コーナーもモデルごとのBOX/CIRCLE指定を優先し、未指定の時だけ従来の斜め線判定を使う。
                if (cornerInfo.type == COLL_CIRCLE)
                {
                    float cx = CellToWorldCenter(x);
                    float cz = CellToWorldCenter(z);
                    if (CircleHitCircle(worldX, worldZ, radius, cx, cz, cornerInfo.radius))
                        return true;

                    continue;
                }
                else if (cornerInfo.type == COLL_BOX)
                {
                    float bx = CellToWorldCenter(x);
                    float bz = CellToWorldCenter(z);
                    if (CircleHitBox(worldX, worldZ, radius, bx, bz, cornerInfo.width, cornerInfo.depth))
                        return true;

                    continue;
                }
                else if (cornerInfo.type == COLL_ARC)
                {
                                        // 2026-05-20: エディター表示と同じく、道側に寄りすぎたコーナーARC判定を弧側へ一段外へ出す。
                    if (CircleHitArc(worldX, worldZ, radius, x, z, GameCornerRot[y][z][x], cornerInfo.radius + 40.0f, max(10.0f, cornerInfo.width)))
                        return true;

                    continue;
                }

                float x1, z1;
                float x2, z2;

                float inset = BLOCK_SIZE * 0.12f;

                switch (GameCornerRot[y][z][x] % 4)
                {
                case 0: // ＼
                    x1 = left + inset;
                    z1 = top + inset;

                    x2 = right - inset;
                    z2 = bottom - inset;
                    break;

                case 1: // ／
                    x1 = right - inset;
                    z1 = top + inset;

                    x2 = left + inset;
                    z2 = bottom - inset;
                    break;

                case 2: // ＼
                    x1 = right - inset;
                    z1 = bottom - inset;

                    x2 = left + inset;
                    z2 = top + inset;
                    break;

                case 3: // ／
                    x1 = left + inset;
                    z1 = bottom - inset;

                    x2 = right - inset;
                    z2 = top + inset;
                    break;
                }

                // 2026-05-13: エディターで調整したコーナー当たり判定の長さ・厚み・奥行を実判定へ反映するため追加。
                float centerX = (x1 + x2) * 0.5f;
                float centerZ = (z1 + z2) * 0.5f;
                float rate = GameCollisionCornerScaleMap[y][z][x] / 100.0f;

                x1 = centerX + (x1 - centerX) * rate;
                z1 = centerZ + (z1 - centerZ) * rate;
                x2 = centerX + (x2 - centerX) * rate;
                z2 = centerZ + (z2 - centerZ) * rate;

                float lineDX = x2 - x1;
                float lineDZ = z2 - z1;
                float lineLen = sqrtf(lineDX * lineDX + lineDZ * lineDZ);
                if (lineLen > 0.0001f)
                {
                    float nx = -lineDZ / lineLen;
                    float nz = lineDX / lineLen;
                    float offset = (float)GameCollisionCornerOffsetMap[y][z][x];

                    x1 += nx * offset;
                    z1 += nz * offset;
                    x2 += nx * offset;
                    z2 += nz * offset;
                }

                float dist = PointLineDistance(
                    worldX,
                    worldZ,
                    x1,
                    z1,
                    x2,
                    z2);

                float wallThickness = BLOCK_SIZE * 0.07f;
                wallThickness *= GameCollisionCornerThicknessMap[y][z][x] / 100.0f;
                if (dist <= radius + wallThickness)
                {
                    return true;
                }
            }




            int decoId = GameDecoMap[y][z][x];

            if (decoId >= 0)
            {
                if (IsLowPlatformDecoId(decoId))
                {
                    float platformTopY = GetVisibleBlockTopY(y, GetLowPlatformTopOffset());
                    float platformBottomY = GetVisibleBlockBottomY(y, GetLowPlatformTopOffset(), LOW_PLATFORM_VISIBLE_THICKNESS);
                    CollisionInfo& info = collisionTable[decoId];
                    float bx = CellToWorldCenter(x);
                    float bz = CellToWorldCenter(z);

                    if (currentY >= platformBottomY - 1.0f && currentY < platformTopY + 1.0f &&
                        CircleHitBox(worldX, worldZ, radius, bx, bz, info.width, info.depth))
                    {
                        // 半マスブロック側面として衝突
                        return true;
                    }

                    // 上面に乗れる高さなら、装飾6の汎用BOX判定では止めない。
                    continue;
                }

                if (IsOverheadDecoId(decoId))
                {
                    // 全体を箱判定にすると下をくぐれないので、装飾7は四隅の柱だけ判定する。
                    float left = x * BLOCK_SIZE;
                    float right = (x + 1) * BLOCK_SIZE;
                    float top = z * BLOCK_SIZE;
                    float bottom = (z + 1) * BLOCK_SIZE;
                    float offset = GetHighPlatformPostOffset();
                    float postRadius = GetHighPlatformPostRadius();

                    if (CircleHitCircle(worldX, worldZ, radius, left + offset, top + offset, postRadius) ||
                        CircleHitCircle(worldX, worldZ, radius, right - offset, top + offset, postRadius) ||
                        CircleHitCircle(worldX, worldZ, radius, left + offset, bottom - offset, postRadius) ||
                        CircleHitCircle(worldX, worldZ, radius, right - offset, bottom - offset, postRadius))
                    {
                        return true;
                    }

                    continue;
                }

                CollisionInfo& info = collisionTable[decoId];

                if (info.type == COLL_CIRCLE)
                {
                    float cx =
                        CellToWorldCenter(x);

                    float cz =
                        CellToWorldCenter(z);

                    if (CircleHitCircle(
                        worldX,
                        worldZ,
                        radius,
                        cx,
                        cz,
                        info.radius))
                    {
                        return true;
                    }
                }
                else if (info.type == COLL_BOX)
                {
                    float bx =
                        CellToWorldCenter(x);

                    float bz =
                        CellToWorldCenter(z);

                    if (CircleHitBox(
                        worldX,
                        worldZ,
                        radius,
                        bx,
                        bz,
                        info.width,
                        info.depth))
                    {
                        return true;
                    }
                }
                else if (info.type == COLL_STAIRS)
                {
                    // 2026-05-20: 階段の横壁判定は不要なので、階段デコ自体では横を塞がない。
                    continue;
                }
            }
        }
    }

    return false;
}
/*
static bool HitWallEdges(int y, float worldX, float worldZ, float radius)
{
    int cellX = WorldToCell(worldX);
    int cellZ = WorldToCell(worldZ);

    for (int dz = -1; dz <= 1; dz++)
    {
        for (int dx = -1; dx <= 1; dx++)
        {
            int x = cellX + dx;
            int z = cellZ + dz;

            if (!IsGameMapPosValid(y, z, x))
                continue;

            float left = x * BLOCK_SIZE;
            float right = (x + 1) * BLOCK_SIZE;
            float top = z * BLOCK_SIZE;
            float bottom = (z + 1) * BLOCK_SIZE;

            if (HasWallEdge(y, z, x, 0) && HitHorizontalWallLine(worldX, worldZ, radius, top, left, right))
                return true;

            if (HasWallEdge(y, z, x, 1) && HitVerticalWallLine(worldX, worldZ, radius, right, top, bottom))
                return true;

            if (HasWallEdge(y, z, x, 2) && HitHorizontalWallLine(worldX, worldZ, radius, bottom, left, right))
                return true;

            if (HasWallEdge(y, z, x, 3) && HitVerticalWallLine(worldX, worldZ, radius, left, top, bottom))
                return true;
        }
    }

    return false;
}
*/

#pragma endregion




#pragma region ===== 形状判定 =====

static bool IsCollisionModelId(int id)
{
    if (id < 0)
        return false;

    if (id == 0)
        return false;

    if (IsStairsId(id))
        return false;

    if (IsLowPlatformDecoId(id) || IsOverheadDecoId(id))
        return false;

    if (id >= MODEL_MAX)
        return false;

    return collisionTable[id].type != COLL_NONE;
}

static bool HitPlacedModelSphere(int id, int tab, int x, int y, int z, int rot, VECTOR center, float radius)
{
    if (!IsCollisionModelId(id))
        return false;

    if (id >= MODEL_MAX || gameModelHandles[id] == -1)
        return false;

    MV1SetPosition(gameModelHandles[id], GetModelDrawPosition(tab, x, y, z, rot));
    MV1SetRotationXYZ(gameModelHandles[id], VGet(0.0f, RotToRad(rot), 0.0f));

    MV1_COLL_RESULT_POLY_DIM hit = MV1CollCheck_Sphere(gameModelHandles[id], -1, center, radius);
    bool result = hit.HitNum > 0;
    MV1CollResultPolyDimTerminate(hit);

    return result;
}
/*
static bool HitCollisionModels(int y, float worldX, float worldZ, float radius)
{
    int cellX = WorldToCell(worldX);
    int cellZ = WorldToCell(worldZ);
    VECTOR center = VGet(worldX, y * BLOCK_SIZE + radius, worldZ);

    for (int dz = -1; dz <= 1; dz++)
    {
        for (int dx = -1; dx <= 1; dx++)
        {
            int x = cellX + dx;
            int z = cellZ + dz;

            if (!IsGameMapPosValid(y, z, x))
                continue;

            // 壁はエディターと同じ端ライン判定を使う。
            // モデル判定も重ねると、モデル原点や当たり情報の差で見た目よりズレることがある。
            //if (HitPlacedModelSphere(GameCornerMap[y][z][x], CORNER, x, y, z, GameCornerRot[y][z][x], center, radius))
            //    return true;

            if (HitPlacedModelSphere(GameDecoMap[y][z][x], DECO, x, y, z, GameDecoRot[y][z][x], center, radius))
                return true;
        }
    }

    return false;
}
*/
#pragma endregion


#pragma region ===== 高さ / 階段 =====

static bool GetStairsProgressInLayer(int y, float worldX, float worldZ, float& progress)
{
    // 2026-05-25: 階段自体に上る場所/下がる場所を持たせるため、配置回転から階段上の進行率を計算する。
    int x = WorldToCell(worldX);
    int z = WorldToCell(worldZ);

    if (!IsMapPosValid(y, z, x))
        return false;

    if (!IsStairsId(GameDecoMap[y][z][x]))
        return false;

    float localX = Clamp01((worldX - x * BLOCK_SIZE) / BLOCK_SIZE);
    float localZ = Clamp01((worldZ - z * BLOCK_SIZE) / BLOCK_SIZE);

    switch (GameDecoRot[y][z][x] % 4)
    {
    case 0: progress = 1.0f - localZ; break;
    case 1: progress = 1.0f - localX; break;
    case 2: progress = localZ; break;
    case 3: progress = localX; break;
    default: progress = 0.0f; break;
    }

    return true;
}

bool IsStairsAtWorldLayer(int layer, float worldX, float worldZ)
{
    // 2026-07-16: 階段機能はいったん使わないため、外部から見ても階段なし扱いにする。
    return false;
}

static bool IsStairsLowerEntry(int y, float worldX, float worldZ)
{
    float progress = 0.0f;
    return GetStairsProgressInLayer(y, worldX, worldZ, progress) && progress <= 0.28f;
}

static bool IsStairsUpperEntry(int y, float worldX, float worldZ)
{
    float progress = 0.0f;
    return GetStairsProgressInLayer(y, worldX, worldZ, progress) && progress >= 0.72f;
}

bool TryGetStairsLoadLayer(int currentLayer, float worldX, float worldZ, int& loadLayer)
{
    // 2026-07-16: 階段機能はいったん使わないため、階段による先読み層切替はしない。
    loadLayer = currentLayer;
    return false;
}
bool TryMoveLayerByStairs(int& layer, float worldX, float worldZ)
{
    // 2026-07-16: 階段機能はいったん使わないため、階段では層を切り替えない。
    return false;
}
bool IsStairsAtWorld(float worldX, float worldZ)
{
    // 2026-07-16: 階段機能はいったん使わないため、ゲーム中の階段判定は常に無効にする。
    return false;
}

float GetMapGroundY(float worldX, float worldZ, float currentY, float groundRadius)
{
    int x = WorldToCell(worldX);
    int z = WorldToCell(worldZ);

    if (x < 0 || x >= BLOCK_NUM_X || z < 0 || z >= BLOCK_NUM_Z)
        return 0.0f;

    int loadedLayer = GetGameLoadedLayer();
    for (int y = loadedLayer; y <= loadedLayer; y++)
    {
        float baseY = y * BLOCK_SIZE;

        // 2026-05-25: 階層ロード後に別階層の床へ吸われないよう、現在の高さから遠い上階は地面候補から外す。
        if (baseY > currentY + BLOCK_SIZE * 0.1f)
            continue;

        // 装飾7の上面は1つ上の階の床。ただし下をくぐっている時に上へ吸い上がらないよう、
        // 現在Yが上面に近い時だけ床として返す。
        if (FindOverheadFloorAtWorld(y, worldX, worldZ) &&
            currentY >= baseY - BLOCK_SIZE * 0.5f)
        {
            return baseY;
        }
        // 2026-07-16: 階段機能はいったん使わないため、階段は地面高さの候補にしない。
        //2026 - 07-10 段差条件のため追加
        if (FindLowPlatformAtWorld(y, worldX, worldZ, groundRadius))
        {
            float platformY = baseY + GetLowPlatformTopOffset();

            if (currentY >= platformY - 80.0f)
            {
                return platformY;
            }

            //return baseY + GetLowPlatformTopOffset();
        }

        if (GameFloorMap[y][z][x] >= 0)
            return baseY;
    }

    return 0.0f;
}
float GetMapGroundY(float worldX, float worldZ, float currentY)
{
    return GetMapGroundY(worldX, worldZ, currentY, 0.0f);
}
float GetMapGroundY(float worldX, float worldZ)
{
    return GetMapGroundY(worldX, worldZ, BLOCK_SIZE * (BLOCK_NUM_Y + 1), 0.0f);
}
#pragma endregion


#pragma region ===== 連続移動判定 =====

static float playerVerticalJumpBaseY = 0.0f;
static float playerLastHorizontalMoveX = 0.0f;
static float playerLastHorizontalMoveZ = 0.0f;


static bool CanStandPlayerPosition(
    int y,
    float worldX,
    float worldZ,
    float radius,
    float currentY)
{
    if (!HasWalkableSurfaceForCircle(y, worldX, worldZ, radius))
        return false;

    if (HitMapObjects(y, worldX, worldZ, radius, currentY))
        return false;

    return true;
}

bool CanCameraMoveWorldPosition(int y, float worldX, float worldZ, float radius, float currentY)
{
    if (!IsMapPosValid(y, WorldToCell(worldZ), WorldToCell(worldX)))
        return true;

    // 2026-07-13: カメラは床の上を歩く物体ではないので、床セル外でも壁・装飾に当たらなければ通す。
    return !HitMapObjects(y, worldX, worldZ, radius, currentY);
}

bool CanMoveWorldPosition(int y, float worldX, float worldZ, float radius, float currentY)
{
    // 2026-05-11: デバッグや他クラスから、ワールド座標が移動可能か直接確認できるよう追加。
    return CanStandPlayerPosition(y, worldX, worldZ, radius, currentY);
}
bool CanMoveWorldPosition(int y, float worldX, float worldZ, float radius)
{
    return CanMoveWorldPosition(y, worldX, worldZ, radius, y * BLOCK_SIZE);
}
static bool CanStandPlayerPositionByWorld(float worldX, float worldZ, float radius, float currentY ,int currentLayer)
{
    // 2026-07-15: ドラクエ風の1層ずつ切り替えに合わせ、通常移動判定も現在層だけを見る。
    return CanStandPlayerPosition(currentLayer, worldX, worldZ, radius, currentY);
}

VECTOR ResolvePlayerMapCollision(
    VECTOR currentPos,
    VECTOR nextPos,
    float radius,
    float currentY,
    int currentLayer)
{
    VECTOR result = currentPos;

    /// 移動量
    float moveX = nextPos.x - currentPos.x;
    float moveZ = nextPos.z - currentPos.z;



    if (CanStandPlayerPositionByWorld(
        nextPos.x,
        nextPos.z,
        radius,
        currentY,
        currentLayer))
    {
        result.x = nextPos.x;
        result.z = nextPos.z;
    }
    else
    {
        if (CanStandPlayerPositionByWorld(
            currentPos.x + moveX,
            currentPos.z,
            radius,
            currentY,
            currentLayer))
        {
            result.x = currentPos.x + moveX;
        }

        if (CanStandPlayerPositionByWorld(
            result.x,
            currentPos.z + moveZ,
            radius,
            currentY,
            currentLayer))
        {
            result.z = currentPos.z + moveZ;
        }
    }

    /// 高さ
    result.y = nextPos.y;

    return result;
}

VECTOR ResolvePlayerMapCollision(
    VECTOR currentPos,
    VECTOR nextPos,
    float radius,
    int currentLayer)
{
    return ResolvePlayerMapCollision(currentPos, nextPos, radius, currentPos.y, currentLayer);
}

// 2026-07-15: 床ブロックの下面・側面判定で、床セルだけを探すため追加。
static bool IsCircleTouchingFloorCell(int y, float worldX, float worldZ, float radius)
{
    int cellX = WorldToCell(worldX);
    int cellZ = WorldToCell(worldZ);
    float half = BLOCK_SIZE * 0.5f;

    for (int dz = -1; dz <= 1; dz++)
    {
        for (int dx = -1; dx <= 1; dx++)
        {
            int x = cellX + dx;
            int z = cellZ + dz;
            if (!IsMapPosValid(y, z, x))
                continue;

            bool hasFloor = GameFloorMap[y][z][x] >= 0;
            if (!hasFloor && y > 0)
                hasFloor = IsOverheadDecoId(GameDecoMap[y - 1][z][x]);

            if (!hasFloor)
                continue;

            float bx = x * BLOCK_SIZE + half;
            float bz = z * BLOCK_SIZE + half;
            if (CircleHitBox(worldX, worldZ, radius, bx, bz, half, half))
                return true;
        }
    }

    return false;
}

// 2026-07-15: 床下面判定は側面のかすりを拾わないよう、プレイヤー中心が床セル下にある時だけ使う。
static bool IsPointUnderFloorCell(int y, float worldX, float worldZ)
{
    int x = WorldToCell(worldX);
    int z = WorldToCell(worldZ);
    if (!IsMapPosValid(y, z, x))
        return false;

    if (GameFloorMap[y][z][x] >= 0)
        return true;

    return y > 0 && IsOverheadDecoId(GameDecoMap[y - 1][z][x]);
}
// 2026-07-15: 完全に1層ずつ判定するため、ロード中ではない上階床ブロック側面は現在層の壁扱いにしない。
static bool IsBlockedByUpperFloorSide(int currentLayer, float worldX, float worldZ, float radius, float currentY, bool canMoveToUpperLayer)
{
    return false;
}

static bool IsLowerClimbHintLandingCell(int currentLayer, float worldX, float worldZ)
{
    if (currentLayer <= 0)
        return false;

    int lowerLayer = currentLayer - 1;
    int x = WorldToCell(worldX);
    int z = WorldToCell(worldZ);
    if (!IsMapPosValid(lowerLayer, z, x))
        return false;

    for (int dir = 0; dir < 4; dir++)
    {
        if (!IsClimbHintDirection(lowerLayer, z, x, dir))
            continue;

        int dx = 0;
        int dz = 0;
        GetClimbHintDirOffset(dir, dx, dz);
        int upperX = x + dx;
        int upperZ = z + dz;
        if (IsMapPosValid(currentLayer, upperZ, upperX) && GameFloorMap[currentLayer][upperZ][upperX] >= 0)
            return true;
    }

    return false;
}
static bool HasPlayerMoveSurface(int currentLayer, float worldX, float worldZ, float radius, float currentY, bool canMoveToUpperLayer)
{
    // 2026-07-15: 当たり判定も完全に1層ずつにするため、通常は現在層だけを見る。
    if (HasWalkableSurfaceForCircle(currentLayer, worldX, worldZ, radius))
        return true;

    if (FindLowPlatformAtWorld(currentLayer, worldX, worldZ, radius))
        return true;

    // 2026-07-16: 上層から黄色マーク元の0.5床へ戻る時だけ、下層の足場へ横移動できるようにする。
    return IsLowerClimbHintLandingCell(currentLayer, worldX, worldZ);
}

static bool IsMovingDownToLowerSurface(int currentLayer, float worldX, float worldZ, float radius, float currentY)
{
    // 2026-07-15: 完全に1層ずつ判定するため、下層への横移動許可はここでは扱わない。
    return false;
}
static bool IsMovingOntoUpperFloor(int currentLayer, float worldX, float worldZ, float radius, float currentY, bool canMoveToUpperLayer)
{
    // 2026-07-15: 完全に1層ずつ判定するため、上層床への横移動許可は層切替後に扱う。
    return false;
}
// 2026-07-15: プレイヤー横移動は現在層の床・壁・装飾だけで判定する。
static bool CanPlayerMoveHorizontally(int y, float worldX, float worldZ, float radius, float currentY, bool canMoveToUpperLayer)
{
    int cellX = WorldToCell(worldX);
    int cellZ = WorldToCell(worldZ);
    if (cellX < 0 || cellX >= BLOCK_NUM_X || cellZ < 0 || cellZ >= BLOCK_NUM_Z)
        return false;

    if (!HasPlayerMoveSurface(y, worldX, worldZ, radius, currentY, canMoveToUpperLayer))
    {
        // 2026-07-15: 現在層に床・0.5段差がない空マスへは横移動できないようにする。
        return false;
    }

    if (IsBlockedByUpperFloorSide(y, worldX, worldZ, radius, currentY, canMoveToUpperLayer))
        return false;
    if (HitMapObjects(y, worldX, worldZ, radius, currentY))
    {
        // 2026-07-15: 当たり判定を完全に1層ずつにするため、横移動では現在層の壁・装飾だけを見る。
        return false;
    }

    if (HitActiveLockedDoorObject(y, worldX, worldZ, radius))
    {
        // 2026-07-21: 鍵扉は開錠されるまで移動判定でも壁として扱う。
        return false;
    }

    return true;
}
VECTOR ResolvePlayerMapCollisionForPlayer(VECTOR currentPos, VECTOR nextPos, float radius, float jumpY, int currentLayer)
{
    float layerBaseY = currentLayer * BLOCK_SIZE;
    float collisionHeight = jumpY;
    if (currentPos.y - layerBaseY > collisionHeight)
        collisionHeight = currentPos.y - layerBaseY;
    if (collisionHeight < 0.0f)
        collisionHeight = 0.0f;

    // 2026-07-15: 横判定も現在の足元高さを使い、同じ層内だけで壁・足場を確認する。
    float currentY = layerBaseY + collisionHeight;
    // 2026-07-15: 以前の上層判定フラグは引数互換のため残すが、現在は完全に1層ずつ判定する。
    bool canMoveToUpperLayer = false;
    VECTOR result = currentPos;
    float moveX = nextPos.x - currentPos.x;
    float moveZ = nextPos.z - currentPos.z;

    // 2026-07-17: プレイヤー側を変更せず、降り口の外側へ進もうとしているかを判定するため移動方向を記録する。
    float moveLen = sqrtf(moveX * moveX + moveZ * moveZ);
    if (moveLen > 0.001f)
    {
        playerLastHorizontalMoveX = moveX / moveLen;
        playerLastHorizontalMoveZ = moveZ / moveLen;
    }
    else
    {
        playerLastHorizontalMoveX = 0.0f;
        playerLastHorizontalMoveZ = 0.0f;
    }

    if (CanPlayerMoveHorizontally(currentLayer, nextPos.x, nextPos.z, radius, currentY, canMoveToUpperLayer))
    {
        result.x = nextPos.x;
        result.z = nextPos.z;
    }
    else
    {
        if (CanPlayerMoveHorizontally(currentLayer, currentPos.x + moveX, currentPos.z, radius, currentY, canMoveToUpperLayer))
            result.x = currentPos.x + moveX;

        if (CanPlayerMoveHorizontally(currentLayer, result.x, currentPos.z + moveZ, radius, currentY, canMoveToUpperLayer))
            result.z = currentPos.z + moveZ;
    }

    result.y = nextPos.y;
    return result;
}

static bool IsPlayerNearClimbHintEdge(int x, int z, int dir, float worldX, float worldZ)
{
    float localX = Clamp01((worldX - x * BLOCK_SIZE) / BLOCK_SIZE);
    float localZ = Clamp01((worldZ - z * BLOCK_SIZE) / BLOCK_SIZE);
    const float edgeThreshold = 0.34f;

    switch (dir & 3)
    {
    case 0: return localZ <= edgeThreshold;
    case 1: return localX >= 1.0f - edgeThreshold;
    case 2: return localZ >= 1.0f - edgeThreshold;
    case 3: return localX <= edgeThreshold;
    }

    return false;
}

static bool IsPlayerOnClimbDownHintMark(int x, int z, int dir, float worldX, float worldZ, float groundRadius)
{
    // 2026-07-17: 中心点だけだと赤い降り口線に触れても降りられないため、プレイヤーの足元円と表示線の接触で判定する。
    float left = x * BLOCK_SIZE;
    float right = (x + 1) * BLOCK_SIZE;
    float front = z * BLOCK_SIZE;
    float back = (z + 1) * BLOCK_SIZE;
    float inset = BLOCK_SIZE * 0.28f;
    float strip = BLOCK_SIZE * 0.07f;
    float x1 = left + inset;
    float x2 = right - inset;
    float z1 = front + inset;
    float z2 = back - inset;

    switch (dir & 3)
    {
    case 0:
        z1 = front + strip;
        z2 = front + strip * 2.0f;
        break;
    case 1:
        x1 = right - strip * 2.0f;
        x2 = right - strip;
        break;
    case 2:
        z1 = back - strip * 2.0f;
        z2 = back - strip;
        break;
    case 3:
        x1 = left + strip;
        x2 = left + strip * 2.0f;
        break;
    }

    return CircleHitAABB(worldX, worldZ, groundRadius, x1, z1, x2, z2);
}
static bool TryClimbToUpperFloorFromHalfStep(VECTOR& pos, float& jumpY, float& vy, bool& isGround, int& layer, float groundRadius)
{
    /*if (isGround || !IsMapHalfStepGroundY(playerVerticalJumpBaseY))
        return false;*/// ジャンプ高さチェックをしない

    if (!IsMapHalfStepGroundY(playerVerticalJumpBaseY))
        return false;

    
    int baseLayer = GetMapGroundLayerFromY(playerVerticalJumpBaseY);
    if (baseLayer + 1 >= BLOCK_NUM_Y)
        return false;

    int x = WorldToCell(pos.x);
    int z = WorldToCell(pos.z);
    if (!IsMapPosValid(baseLayer, z, x))
        return false;

    float upperGroundY = (baseLayer + 1) * BLOCK_SIZE;
    /*float targetFootY = playerVerticalJumpBaseY + jumpY;
    if (targetFootY < upperGroundY - 8.0f)
        return false;*/// ジャンプ高さチェックをしない


    for (int dir = 0; dir < 4; dir++)
    {
        if (!IsClimbHintDirection(baseLayer, z, x, dir))
            continue;

        if (!IsPlayerNearClimbHintEdge(x, z, dir, pos.x, pos.z))
            continue;

        int dx = 0;
        int dz = 0;
        GetClimbHintDirOffset(dir, dx, dz);
        int targetX = x + dx;
        int targetZ = z + dz;
        int upperLayer = baseLayer + 1;

        float targetXWorld = CellToWorldCenter(targetX);
        float targetZWorld = CellToWorldCenter(targetZ);
        if (!CanStandPlayerPosition(upperLayer, targetXWorld, targetZWorld, groundRadius, upperGroundY))
            continue;

        // 2026-07-16: 0.5床のマーク方向へ跳んだ時だけ、隣の上層床へ着地させる。
        pos.x = targetXWorld;
        pos.z = targetZWorld;
        pos.y = upperGroundY;
        layer = upperLayer;
        jumpY = 0.0f;
        vy = 0.0f;
        isGround = true;
        playerVerticalJumpBaseY = upperGroundY;
        SetGameLoadedLayer(layer);
        return true;
    }

    return false;
}
static bool TryStepDownToLowerHalfStep(VECTOR& pos, float& jumpY, float& vy, bool& isGround, int& layer, float groundRadius)
{
    if (!isGround || layer <= 0)
        return false;

    int upperX = WorldToCell(pos.x);
    int upperZ = WorldToCell(pos.z);
    if (!IsMapPosValid(layer, upperZ, upperX))
        return false;

    for (int dir = 0; dir < 4; dir++)
    {
        if (!IsClimbDownHintDirection(layer, upperZ, upperX, dir))
            continue;

        if (!IsPlayerOnClimbDownHintMark(upperX, upperZ, dir, pos.x, pos.z, groundRadius))
            continue;

        int dx = 0;
        int dz = 0;
        GetClimbHintDirOffset(dir, dx, dz);

        // 2026-07-17: ブロック端にいるだけでは降ろさず、その端の外側へ進もうとした時だけ降ろす。
        float moveDot = playerLastHorizontalMoveX * dx + playerLastHorizontalMoveZ * dz;
        if (moveDot < 0.45f)
            continue;

        int lowerLayer = layer - 1;
        int lowerX = upperX + dx;
        int lowerZ = upperZ + dz;
        float targetXWorld = CellToWorldCenter(lowerX);
        float targetZWorld = CellToWorldCenter(lowerZ);

        if (!FindLowPlatformAtWorld(lowerLayer, targetXWorld, targetZWorld, groundRadius))
            continue;

        float lowerGroundY = lowerLayer * BLOCK_SIZE + GetLowPlatformTopOffset();

        // 2026-07-16: 上層床の降り口側の端に来たら、対応する下の0.5床へ降ろす。
        layer = lowerLayer;
        pos.x = targetXWorld;
        pos.z = targetZWorld;
        pos.y = lowerGroundY;
        jumpY = 0.0f;
        vy = 0.0f;
        isGround = true;
        playerVerticalJumpBaseY = lowerGroundY;
        SetGameLoadedLayer(layer);
        return true;
    }

    return false;
}
// 2026-07-15: 上昇中に上階床の下面へ頭をぶつけるため、プレイヤー半径込みで床セルを天井として探す。
static float FindPlayerCeilingY(float worldX, float worldZ, float radius, float currentY)
{
    int loadedLayer = GetGameLoadedLayer();
    for (int y = loadedLayer; y <= loadedLayer; y++)
    {
        float ceilingY = GetVisibleBlockTopY(y, 0.0f);
        if (ceilingY <= currentY + 1.0f)
            continue;

        if (ceilingY > currentY + BLOCK_SIZE * 1.25f)
            break;

        if (IsCircleTouchingFloorCell(y, worldX, worldZ, radius))
        {
            // 2026-07-15: 中心点だけだと肩側が上階床の下面へめり込むため、横移動と同じ半径で天井を拾う。
            return ceilingY;
        }
    }

    return -1.0f;
}
void UpdatePlayerMapVertical(VECTOR& pos, float& jumpY, float& vy, bool& isGround, int& layer, bool jumpTrigger, bool stepDownTrigger, float gravity, float groundRadius)
{
    float layerBaseY = layer * BLOCK_SIZE;
    float standingGroundY = GetMapGroundY(pos.x, pos.z, pos.y, groundRadius);

    if (isGround)
    {
        // 2026-07-17: 降り口に触れただけで降りないよう、ブロック端の外側へ進もうとした時だけ0.5床へ降ろす。
        if (TryStepDownToLowerHalfStep(pos, jumpY, vy, isGround, layer, groundRadius))
            return;

        if (standingGroundY < pos.y - 20.0f)
        {
            // 2026-07-13: 床端から出た時は下の床へ吸着せず、現在高さから落下を始める。
            isGround = false;
            playerVerticalJumpBaseY = pos.y;
            jumpY = 0.0f;
            vy = 0.0f;
        }
        else
        {
            layer = GetMapGroundLayerFromY(standingGroundY);
            layerBaseY = layer * BLOCK_SIZE;
            playerVerticalJumpBaseY = standingGroundY;
            jumpY = 0.0f;
            vy = 0.0f;
        }
    }

    if (jumpTrigger && isGround)
    {
        playerVerticalJumpBaseY = standingGroundY;
        vy = 20.0f;
        isGround = false;
    
    }

    if (!isGround)
    {
        vy += gravity;
        jumpY += vy;

        // 2026-07-15: 上面より高く上がった後までジャンプ開始高さで天井判定すると、床下面へ吸い戻されるため現在の足元高さで見る。
        if (vy > 0.0f)
        {
            float ceilingY = FindPlayerCeilingY(pos.x, pos.z, groundRadius, pos.y);
            if (ceilingY >= 0.0f)
            {
                float targetFootY = playerVerticalJumpBaseY + jumpY;
                int ceilingLayer = GetMapGroundLayerFromY(ceilingY);
                float ceilingGroundY = GetVisibleBlockTopY(ceilingLayer, 0.0f);
                bool canStepOntoUpperFloor =
                    IsMapHalfStepGroundY(playerVerticalJumpBaseY) &&
                    targetFootY >= ceilingGroundY - 1.0f &&
                    IsCircleTouchingFloorCell(ceilingLayer, pos.x, pos.z, groundRadius);

                if (!canStepOntoUpperFloor)
                {
                    // 2026-07-15: 足元Yを床下面ぴったりまで上げるとプレイヤーの半径ぶん天井へめり込むため、半径ぶん手前で止める。
                    float ceilingBottomY = ceilingY - FLOOR_BLOCK_VISIBLE_THICKNESS;
                    float maxFootY = ceilingBottomY - groundRadius;
                    if (targetFootY > maxFootY)
                    {
                        jumpY = maxFootY - playerVerticalJumpBaseY;
                        if (jumpY < 0.0f)
                            jumpY = 0.0f;
                        vy = 0.0f;
                    }
                }
            }
        }
    }

    if (TryClimbToUpperFloorFromHalfStep(pos, jumpY, vy, isGround, layer, groundRadius))
        return;

    float groundProbeHeight = jumpY;
    if (pos.y - layerBaseY > groundProbeHeight)
        groundProbeHeight = pos.y - layerBaseY;
    if (groundProbeHeight > LOW_PLATFORM_VISIBLE_THICKNESS)
        groundProbeHeight = LOW_PLATFORM_VISIBLE_THICKNESS;
    if (groundProbeHeight < 0.0f)
        groundProbeHeight = 0.0f;

    float groundProbeY = layerBaseY + groundProbeHeight;
    if (!isGround && IsMapHalfStepGroundY(playerVerticalJumpBaseY))
    {
        float jumpWorldY = playerVerticalJumpBaseY + jumpY;
        if (jumpWorldY > groundProbeY)
            groundProbeY = jumpWorldY;
    }

    float groundY = GetMapGroundY(pos.x, pos.z, groundProbeY, groundRadius);
    // 2026-07-15: 完全に1層ずつ判定するため、0.5段差ジャンプ中でも上層床は層切替後まで着地候補にしない。

    if (!isGround && IsMapHalfStepGroundY(playerVerticalJumpBaseY) && groundY < playerVerticalJumpBaseY)
    {
        // 2026-07-15: 0.5段差からの上昇が足りない時、下床へ吸われず元の段差へ戻れるようにする。
        float baseGroundY = GetMapGroundY(pos.x, pos.z, playerVerticalJumpBaseY, 0.0f);
        if (IsMapHalfStepGroundY(baseGroundY))
            groundY = baseGroundY;
    }

    float targetY = isGround ? groundY : playerVerticalJumpBaseY + jumpY;

    bool landedOnGround = false;
    if (!isGround && vy <= 0.0f && targetY <= groundY)
    {
        jumpY = 0.0f;
        vy = 0.0f;
        isGround = true;
        playerVerticalJumpBaseY = groundY;
        targetY = groundY;
        layer = GetMapGroundLayerFromY(groundY);
        landedOnGround = true;
    }

    if (!isGround || landedOnGround)
    {
        // 2026-07-15: 空中では補間で見た目だけ遅れると上昇時にワープっぽく見えるため、物理高さをそのまま使う。
        pos.y = targetY;
    }
    else
    {
        pos.y += (targetY - pos.y) * 0.2f;
    }
}

bool IsGoalWorldPosition(int y, float worldX, float worldZ)
{
    int x = WorldToCell(worldX);
    int z = WorldToCell(worldZ);

    return IsGoalCell(y, z, x);
}

// 7/17 階層移動先を取得する関数
bool GetLayerMoveDestination(
    int currentLayer,
    int currentX,
    int currentZ,
    int direction,
    MapNode& destination)
{
    int dx = 0;
    int dz = 0;

    GetClimbHintDirOffset(direction, dx, dz);

    // 下層の0.5床から上層へ移動
    if (IsClimbHintDirection(
        currentLayer,
        currentZ,
        currentX,
        direction))
    {
        int nextLayer = currentLayer + 1;
        int nextX = currentX + dx;
        int nextZ = currentZ + dz;

        if (!IsMapPosValid(nextLayer, nextZ, nextX))
            return false;

        if (!IsWalkableCell(nextLayer, nextZ, nextX))
            return false;

        destination = {
            nextLayer,
            nextX,
            nextZ
        };

        return true;
    }

    // 上層から下層の0.5床へ移動
    if (IsClimbDownHintDirection(
        currentLayer,
        currentZ,
        currentX,
        direction))
    {
        int nextLayer = currentLayer - 1;
        int nextX = currentX + dx;
        int nextZ = currentZ + dz;

        if (!IsMapPosValid(nextLayer, nextZ, nextX))
            return false;

        if (!IsLowPlatformDecoId(
            GameDecoMap[nextLayer][nextZ][nextX]))
        {
            return false;
        }

        destination = {
            nextLayer,
            nextX,
            nextZ
        };

        return true;
    }

    return false;
}

#pragma endregion

























































