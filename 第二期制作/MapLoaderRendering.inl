static bool IsGameCustomColor(int colorIndex)
{
    // 2026-07-21: エディターの自由色は0x01000000|RGBで保存し、ゲーム側でも同じ形式で読む。
    return (colorIndex & 0x01000000) != 0;
}

static COLOR_F GetGamePlacedColorF(int colorIndex)
{
    // 2026-07-21: エディターで保存した色番号を、ゲーム側モデル描画用のマテリアル色へ変換する。
    if (IsGameCustomColor(colorIndex))
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

static VECTOR ApplyGameWallModelAlignmentOffset(int id, int rot, VECTOR pos)
{
    // 2026-07-22: モデル34はいったん外したため、ゲーム中の追加壁個別位置補正は使わない。
    return pos;
}

static int GetGamePlacedDrawColor(int colorIndex, int defaultColor)
{
    // 2026-07-21: モデル下の簡易床にも、保存された色変更を反映する。
    if (IsGameCustomColor(colorIndex))
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
    default: return defaultColor;
    }
}
static void DrawModelById(int id, VECTOR pos, int rot, int colorIndex = 0)
{
    if (id < 0 || id >= MODEL_MAX)
        return;


    if (gameModelHandles[id] == -1)
        return;

    pos = ApplyGameModelConfigOffset(id, pos);
    pos = ApplyGameWallModelAlignmentOffset(id, rot, pos);
    MV1SetPosition(gameModelHandles[id], pos);
    int drawRot = GetGameModelDrawRot(id, rot);
    MV1SetRotationXYZ(gameModelHandles[id], VGet(0.0f, RotToRad(drawRot), 0.0f));

    // 2026-07-21: 配置色は描画時だけマテリアルへ乗せ、同じモデルを別セルで使い回せるようにする。
    // 2026-07-21: MaterialDifColorはテクスチャ付きモデルだと効きにくいため、モデル全体の色倍率で tint する。
    MV1SetDifColorScale(gameModelHandles[id], GetGamePlacedColorF(colorIndex));
    MV1SetAmbColorScale(gameModelHandles[id], GetGamePlacedColorF(colorIndex));

    // 2026-07-21: 床や装飾も角度で暗くならないよう、モデル描画中だけライト計算を切る。
    int oldLighting = GetLightEnable();
    SetUseLighting(FALSE);
    SetLightEnable(FALSE);
    MV1DrawModel(gameModelHandles[id]);
    SetLightEnable(oldLighting);
    SetUseLighting(TRUE);
    MV1SetDifColorScale(gameModelHandles[id], GetColorF(1.0f, 1.0f, 1.0f, 1.0f));
    MV1SetAmbColorScale(gameModelHandles[id], GetColorF(1.0f, 1.0f, 1.0f, 1.0f));
}

static void DrawLitModelById(int id, VECTOR pos, int rot, int colorIndex = 0)
{
    if (id < 0 || id >= MODEL_MAX)
        return;

    if (gameModelHandles[id] == -1)
        return;

    pos = ApplyGameModelConfigOffset(id, pos);
    pos = ApplyGameWallModelAlignmentOffset(id, rot, pos);
    MV1SetPosition(gameModelHandles[id], pos);
    int drawRot = GetGameModelDrawRot(id, rot);
    MV1SetRotationXYZ(gameModelHandles[id], VGet(0.0f, RotToRad(drawRot), 0.0f));

    // 2026-07-21: 壁や角も配置色を描画時だけマテリアルへ乗せる。
    // 2026-07-21: MaterialDifColorはテクスチャ付きモデルだと効きにくいため、モデル全体の色倍率で tint する。
    MV1SetDifColorScale(gameModelHandles[id], GetGamePlacedColorF(colorIndex));
    MV1SetAmbColorScale(gameModelHandles[id], GetGamePlacedColorF(colorIndex));

    // 2026-07-21: 壁や角も床/装飾と同じく、描画中だけライト計算を切って全体を同じ明るさで見せる。
    int oldLighting = GetLightEnable();
    SetUseLighting(FALSE);
    SetLightEnable(FALSE);
    MV1DrawModel(gameModelHandles[id]);
    SetLightEnable(oldLighting);
    SetUseLighting(TRUE);
    MV1SetDifColorScale(gameModelHandles[id], GetColorF(1.0f, 1.0f, 1.0f, 1.0f));
    MV1SetAmbColorScale(gameModelHandles[id], GetColorF(1.0f, 1.0f, 1.0f, 1.0f));
}

//2026-07-14-床ブロックの下に厚みを持たせるためについか
static void DrawGameFloorBlockBase(int x, int y, int z, float topOffset, float height, int faceColor, int edgeColor)
{
    float left = x * BLOCK_SIZE;
    float right = (x + 1) * BLOCK_SIZE;
    float front = z * BLOCK_SIZE;
    float back = (z + 1) * BLOCK_SIZE;
    float topY = y * BLOCK_SIZE + topOffset - 2.0f;
    float bottomY = topY - height;
    

    DrawCube3D(
        VGet(left, bottomY, front),
        VGet(right, topY, back),
        faceColor,
        edgeColor,
        TRUE
    );
}
static void DrawGameClimbHintEdge(int x, int y, int z, int dir)
{
    // 2026-07-21: 上層へ行ける場所は黄色い印ではなく、はしごモデルだけで表示する。
    int dx = 0;
    int dz = 0;
    GetClimbHintDirOffset(dir, dx, dz);

    VECTOR pos = VGet(
        CellToWorldCenter(x) + dx * BLOCK_SIZE * 0.42f,
        y * BLOCK_SIZE + GetLowPlatformTopOffset(),
        CellToWorldCenter(z) + dz * BLOCK_SIZE * 0.42f
    );

    DrawModelById(30, pos, dir);
}

static void DrawGameClimbDownHintEdge(int x, int y, int z, int dir)
{
    // 2026-07-16: 上層床側にも、下の0.5床へ降りられる方向を床の縁マークで示す。
    float left = x * BLOCK_SIZE;
    float right = (x + 1) * BLOCK_SIZE;
    float front = z * BLOCK_SIZE;
    float back = (z + 1) * BLOCK_SIZE;
    float topY = y * BLOCK_SIZE + 4.0f;

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

    DrawCube3D(
        VGet(x1, topY, z1),
        VGet(x2, topY + 5.0f, z2),
        GetColor(255, 150, 70),
        GetColor(125, 54, 25),
        TRUE
    );
}

static void DrawGameClimbHints(int x, int y, int z)
{
    for (int dir = 0; dir < 4; dir++)
    {
        if (IsClimbHintDirection(y, z, x, dir))
            DrawGameClimbHintEdge(x, y, z, dir);
    }
}

static void DrawGameClimbDownHints(int x, int y, int z)
{
    for (int dir = 0; dir < 4; dir++)
    {
        if (IsClimbDownHintDirection(y, z, x, dir))
            DrawGameClimbDownHintEdge(x, y, z, dir);
    }
}

void DrawGameMap()
{
    SetUseZBuffer3D(TRUE);
    SetWriteZBuffer3D(TRUE);

    int fadeHalf = GAME_LAYER_FADE_FRAMES / 2;
    int drawLayer = gameDrawLayer;
    if (gameLayerFadeFrame >= fadeHalf && gameLayerFadeFrame > 0)
        drawLayer = gameLayerFadeNextLayer;

    for (int y = 0; y < BLOCK_NUM_Y; y++)
    {
        // 2026-07-16: 階段機能はいったん使わないため、描画も現在層だけに戻す。
        bool drawFullLayer = (y == drawLayer);
        if (!drawFullLayer)
            continue;

        for (int z = 0; z < BLOCK_NUM_Z; z++)
        {
            for (int x = 0; x < BLOCK_NUM_X; x++)
            {
                if (drawFullLayer && GameFloorMap[y][z][x] >= 0)
                {
                    DrawGameFloorBlockBase(x, y, z, 0.0f, BLOCK_SIZE * 0.22f, GetGamePlacedDrawColor(GameFloorColorMap[y][z][x], GetColor(82, 72, 58)), GetColor(38, 32, 26));
                    DrawModelById(
                        GameFloorMap[y][z][x],
                        GetModelDrawPosition(FLOOR, x, y, z, GameFloorRot[y][z][x]),
                        GameFloorRot[y][z][x],
                        GameFloorColorMap[y][z][x]
                    );
                    DrawGameClimbDownHints(x, y, z);
                }

                if (drawFullLayer && GameWallMapA[y][z][x] >= 0)
                {
                    DrawLitModelById(
                        GameWallMapA[y][z][x],
                        GetModelDrawPosition(WALL, x, y, z, GameWallRotA[y][z][x]),
                        GameWallRotA[y][z][x],
                        GameWallColorMapA[y][z][x]
                    );
                }

                if (drawFullLayer && GameWallMapB[y][z][x] >= 0)
                {
                    DrawLitModelById(
                        GameWallMapB[y][z][x],
                        GetModelDrawPosition(WALL, x, y, z, GameWallRotB[y][z][x]),
                        GameWallRotB[y][z][x],
                        GameWallColorMapB[y][z][x]
                    );
                }

                if (drawFullLayer && GameCornerMap[y][z][x] >= 0)
                {
                    DrawLitModelById(
                        GameCornerMap[y][z][x],
                        GetModelDrawPosition(CORNER, x, y, z, GameCornerRot[y][z][x]),
                        GameCornerRot[y][z][x],
                        GameCornerColorMap[y][z][x]
                    );
                }

                if (drawFullLayer && GameDecoMap[y][z][x] >= 0)
                {
                    DrawModelById(
                        GameDecoMap[y][z][x],
                        GetModelDrawPosition(DECO, x, y, z, GameDecoRot[y][z][x]),
                        GameDecoRot[y][z][x],
                        GameDecoColorMap[y][z][x]
                    );

                    if (IsLowPlatformDecoId(GameDecoMap[y][z][x]))
                    {
                        // 2026-07-22: 段差6はモデルを描いた後に半ブロック箱を重ね、画像のような足場面を前面に出す。
                        DrawGameFloorBlockBase(x, y, z, BLOCK_SIZE * 0.5f, BLOCK_SIZE * 0.5f, GetGamePlacedDrawColor(GameDecoColorMap[y][z][x], GetColor(90, 80, 62)), GetColor(42, 35, 27));
                        DrawGameClimbHints(x, y, z);
                    }
                }
            }
        }
    }
    /*debug用
    DrawFormatString(
        20,
        120,
        GetColor(255, 255, 255),
        "LoadedLayer:%d",
        gameLoadedLayer
    );
    */
    // 2026-07-22: プレイ画面左上に出していたMap/Start/Goal座標のデバッグ文字は提出用に表示しない。

}

void DrawGameLayerFadeOverlay()
{
    if (gameLayerFadeFrame <= 0)
        return;

    int fadeHalf = GAME_LAYER_FADE_FRAMES / 2;
    int alpha = 0;
    if (gameLayerFadeFrame < fadeHalf)
    {
        alpha = gameLayerFadeFrame * 255 / fadeHalf;
    }
    else
    {
        alpha = (GAME_LAYER_FADE_FRAMES - gameLayerFadeFrame) * 255 / (GAME_LAYER_FADE_FRAMES - fadeHalf);
        gameDrawLayer = gameLayerFadeNextLayer;
    }

    if (alpha < 0) alpha = 0;
    if (alpha > 255) alpha = 255;

    int screenW = 0;
    int screenH = 0;
    GetDrawScreenSize(&screenW, &screenH);
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
    DrawBox(0, 0, screenW, screenH, GetColor(0, 0, 0), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    gameLayerFadeFrame++;
    if (gameLayerFadeFrame > GAME_LAYER_FADE_FRAMES)
    {
        gameLayerFadeFrame = 0;
        gameDrawLayer = gameLayerFadeNextLayer;
    }
}

#include "MapLoaderMiniMap.inl"

#pragma endregion


#pragma region ===== セル単位判定 =====




