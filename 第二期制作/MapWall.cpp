#include "MapEditor.h"

#pragma region ===== 壁1マス2個配置 =====

static void UpdateWallSlotTile(int y, int z, int x, int map[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X], int rot[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X])
{
    // 2026-07-15: 壁A/B/C/Dを同じ接続処理で更新する。
    if (map[y][z][x] < 0)
        return;

    bool left = (x > 0 && map[y][z][x - 1] >= 0);
    bool right = (x < BLOCK_NUM_X - 1 && map[y][z][x + 1] >= 0);

    if (left || right)
        rot[y][z][x] = 1;
    else
        rot[y][z][x] = 0;
}

void UpdateWallTile(int y, int z, int x)
{
    if (x < 0 || x >= BLOCK_NUM_X) return;
    if (y < 0 || y >= BLOCK_NUM_Y) return;
    if (z < 0 || z >= BLOCK_NUM_Z) return;

    UpdateWallSlotTile(y, z, x, WallMapA, WallRotA);
    UpdateWallSlotTile(y, z, x, WallMapB, WallRotB);
}
#pragma endregion



#pragma region ===== 全壁更新 =====

void AutoConnectWalls()
{
    for (int y = 0; y < BLOCK_NUM_Y; y++)
    {
        for (int z = 0; z < BLOCK_NUM_Z; z++)
        {
            for (int x = 0; x < BLOCK_NUM_X; x++)
            {
                UpdateWallTile(y, z, x);
            }
        }
    }
}

#pragma endregion

