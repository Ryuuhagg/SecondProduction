#include "MapEditor.h"
#include <vector>
#include <cstring>

using namespace std;

const int UNDO_MAX = 20;

struct UndoData
{
    int FloorMap[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];
    int FloorRot[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];

    int WallMapA[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];
    int WallMapB[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];
    int WallRotA[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];
    int WallRotB[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];

    int CornerMap[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];
    int CornerRot[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];

    int DecoMap[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];
    int DecoRot[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];
    // 2026-07-16: 0.5床の上層接続方向もUndoで戻す。
    int ClimbLinkMap[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];

    // 2026-06-11: EVENTと分離した敵配置もUndoで戻せるよう追加。
    int EnemyMap[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];
    int EnemyPatrolCountMap[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];
    int EnemyPatrolXMap[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X][ENEMY_PATROL_POINT_MAX];
    int EnemyPatrolZMap[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X][ENEMY_PATROL_POINT_MAX];
    int EventMap[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];

    // 2026-05-11: 当たり判定編集もUndoで戻せるよう追加。
    int CollisionMap[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];
    // 2026-05-20: BoxCollider風の手動当たり判定調整もUndoで戻せるよう追加。
    int CollisionBoxOffsetXMap[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];
    int CollisionBoxOffsetZMap[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];
    int CollisionBoxSizeXMap[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];
    int CollisionBoxSizeZMap[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];
    // 2026-05-11: 辺単位の当たり判定編集もUndoで戻せるよう追加。
    int CollisionEdgeMap[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];
    // 2026-05-11: 辺当たり判定の伸縮値もUndoで戻せるよう追加。
    int CollisionEdgeScaleMap[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X][4];
    // 2026-05-11: 辺当たり判定の厚さ編集もUndoで戻せるよう追加。
    int CollisionEdgeThicknessMap[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X][4];
    // 2026-05-13: コーナー当たり判定の調整値もUndoで戻せるよう追加。
    int CollisionCornerScaleMap[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];
    int CollisionCornerThicknessMap[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];
    int CollisionCornerOffsetMap[BLOCK_NUM_Y][BLOCK_NUM_Z][BLOCK_NUM_X];

    int startX;
    int startY;
    int startZ;

    int goalX;
    int goalY;
    int goalZ;
};

vector<UndoData> undoList;

void PushUndo()
{
    // 2026-05-27: Undo履歴が積まれる編集は、未保存変更ありとして扱う。
    MarkEditorMapDirty();
    if ((int)undoList.size() >= UNDO_MAX)
    {
        undoList.erase(undoList.begin());
    }

    undoList.emplace_back();
    UndoData& data = undoList.back();

    memcpy(data.FloorMap, ::FloorMap, sizeof(::FloorMap));
    memcpy(data.FloorRot, ::FloorRot, sizeof(::FloorRot));

    memcpy(data.WallMapA, ::WallMapA, sizeof(::WallMapA));
    memcpy(data.WallMapB, ::WallMapB, sizeof(::WallMapB));
    memcpy(data.WallRotA, ::WallRotA, sizeof(::WallRotA));
    memcpy(data.WallRotB, ::WallRotB, sizeof(::WallRotB));

    memcpy(data.CornerMap, ::CornerMap, sizeof(::CornerMap));
    memcpy(data.CornerRot, ::CornerRot, sizeof(::CornerRot));

    memcpy(data.DecoMap, ::DecoMap, sizeof(::DecoMap));
    memcpy(data.DecoRot, ::DecoRot, sizeof(::DecoRot));
    memcpy(data.ClimbLinkMap, ::ClimbLinkMap, sizeof(::ClimbLinkMap));

    // 2026-06-11: Undo履歴に敵配置を保存するため追加。
    memcpy(data.EnemyMap, ::EnemyMap, sizeof(::EnemyMap));
    // 2026-07-08: 敵ごとの巡回ポイント編集もUndoで戻せるよう保存する。
    memcpy(data.EnemyPatrolCountMap, ::EnemyPatrolCountMap, sizeof(::EnemyPatrolCountMap));
    memcpy(data.EnemyPatrolXMap, ::EnemyPatrolXMap, sizeof(::EnemyPatrolXMap));
    memcpy(data.EnemyPatrolZMap, ::EnemyPatrolZMap, sizeof(::EnemyPatrolZMap));
    memcpy(data.EventMap, ::EventMap, sizeof(::EventMap));
    // 2026-05-11: 当たり判定編集をUndo履歴に保存するため追加。
    memcpy(data.CollisionMap, ::CollisionMap, sizeof(::CollisionMap));
    // 2026-05-20: BoxCollider風の調整値をUndo履歴に保存する。
    memcpy(data.CollisionBoxOffsetXMap, ::CollisionBoxOffsetXMap, sizeof(::CollisionBoxOffsetXMap));
    memcpy(data.CollisionBoxOffsetZMap, ::CollisionBoxOffsetZMap, sizeof(::CollisionBoxOffsetZMap));
    memcpy(data.CollisionBoxSizeXMap, ::CollisionBoxSizeXMap, sizeof(::CollisionBoxSizeXMap));
    memcpy(data.CollisionBoxSizeZMap, ::CollisionBoxSizeZMap, sizeof(::CollisionBoxSizeZMap));
    // 2026-05-11: 辺当たり判定編集をUndo履歴に保存するため追加。
    memcpy(data.CollisionEdgeMap, ::CollisionEdgeMap, sizeof(::CollisionEdgeMap));
    // 2026-05-11: 辺当たり判定の伸縮値をUndo履歴に保存するため追加。
    memcpy(data.CollisionEdgeScaleMap, ::CollisionEdgeScaleMap, sizeof(::CollisionEdgeScaleMap));
    // 2026-05-11: 辺当たり判定の厚さをUndo履歴に保存するため追加。
    memcpy(data.CollisionEdgeThicknessMap, ::CollisionEdgeThicknessMap, sizeof(::CollisionEdgeThicknessMap));
    // 2026-05-13: コーナー当たり判定の調整値をUndo履歴に保存するため追加。
    memcpy(data.CollisionCornerScaleMap, ::CollisionCornerScaleMap, sizeof(::CollisionCornerScaleMap));
    memcpy(data.CollisionCornerThicknessMap, ::CollisionCornerThicknessMap, sizeof(::CollisionCornerThicknessMap));
    memcpy(data.CollisionCornerOffsetMap, ::CollisionCornerOffsetMap, sizeof(::CollisionCornerOffsetMap));

    data.startX = startX;
    data.startY = startY;
    data.startZ = startZ;

    data.goalX = goalX;
    data.goalY = goalY;
    data.goalZ = goalZ;
}

void UndoMap()
{
    if (undoList.empty())
        return;

    const UndoData& data = undoList.back();

    memcpy(::FloorMap, data.FloorMap, sizeof(::FloorMap));
    memcpy(::FloorRot, data.FloorRot, sizeof(::FloorRot));

    memcpy(::WallMapA, data.WallMapA, sizeof(::WallMapA));
    memcpy(::WallMapB, data.WallMapB, sizeof(::WallMapB));
    memcpy(::WallRotA, data.WallRotA, sizeof(::WallRotA));
    memcpy(::WallRotB, data.WallRotB, sizeof(::WallRotB));

    memcpy(::CornerMap, data.CornerMap, sizeof(::CornerMap));
    memcpy(::CornerRot, data.CornerRot, sizeof(::CornerRot));

    memcpy(::DecoMap, data.DecoMap, sizeof(::DecoMap));
    memcpy(::DecoRot, data.DecoRot, sizeof(::DecoRot));
    memcpy(::ClimbLinkMap, data.ClimbLinkMap, sizeof(::ClimbLinkMap));

    // 2026-06-11: Undo実行時に敵配置も復元するため追加。
    memcpy(::EnemyMap, data.EnemyMap, sizeof(::EnemyMap));
    // 2026-07-08: Undo実行時に巡回ポイントも敵配置と一緒に復元する。
    memcpy(::EnemyPatrolCountMap, data.EnemyPatrolCountMap, sizeof(::EnemyPatrolCountMap));
    memcpy(::EnemyPatrolXMap, data.EnemyPatrolXMap, sizeof(::EnemyPatrolXMap));
    memcpy(::EnemyPatrolZMap, data.EnemyPatrolZMap, sizeof(::EnemyPatrolZMap));
    memcpy(::EventMap, data.EventMap, sizeof(::EventMap));
    // 2026-05-11: Undo実行時に当たり判定編集も戻すため追加。
    memcpy(::CollisionMap, data.CollisionMap, sizeof(::CollisionMap));
    // 2026-05-20: Undo実行時にBoxCollider風の調整値も戻す。
    memcpy(::CollisionBoxOffsetXMap, data.CollisionBoxOffsetXMap, sizeof(::CollisionBoxOffsetXMap));
    memcpy(::CollisionBoxOffsetZMap, data.CollisionBoxOffsetZMap, sizeof(::CollisionBoxOffsetZMap));
    memcpy(::CollisionBoxSizeXMap, data.CollisionBoxSizeXMap, sizeof(::CollisionBoxSizeXMap));
    memcpy(::CollisionBoxSizeZMap, data.CollisionBoxSizeZMap, sizeof(::CollisionBoxSizeZMap));
    // 2026-05-11: Undo実行時に辺当たり判定編集も戻すため追加。
    memcpy(::CollisionEdgeMap, data.CollisionEdgeMap, sizeof(::CollisionEdgeMap));
    // 2026-05-11: Undo実行時に辺当たり判定の伸縮値も戻すため追加。
    memcpy(::CollisionEdgeScaleMap, data.CollisionEdgeScaleMap, sizeof(::CollisionEdgeScaleMap));
    // 2026-05-11: Undo実行時に辺当たり判定の厚さも戻すため追加。
    memcpy(::CollisionEdgeThicknessMap, data.CollisionEdgeThicknessMap, sizeof(::CollisionEdgeThicknessMap));
    // 2026-05-13: Undo実行時にコーナー当たり判定の調整値も戻すため追加。
    memcpy(::CollisionCornerScaleMap, data.CollisionCornerScaleMap, sizeof(::CollisionCornerScaleMap));
    memcpy(::CollisionCornerThicknessMap, data.CollisionCornerThicknessMap, sizeof(::CollisionCornerThicknessMap));
    memcpy(::CollisionCornerOffsetMap, data.CollisionCornerOffsetMap, sizeof(::CollisionCornerOffsetMap));

    startX = data.startX;
    startY = data.startY;
    startZ = data.startZ;

    goalX = data.goalX;
    goalY = data.goalY;
    goalZ = data.goalZ;

    undoList.pop_back();
}



