#include"AttackData.h"
#include<cmath>
#include"Constant.h"
bool AttackData::Contains(VECTOR targetPos) const {
    VECTOR toTarget = VSub(targetPos, position);
    toTarget.y = 0.0f;

    float distance = VSize(toTarget);
    // 相手の位置が範囲外だったらfalseを返す
    if (shape != AttackShape::RectAngle &&
        distance > range)
        return false;
    // positionを中心に円形の攻撃
    if (shape == AttackShape::Circle)
        return true;
    if (shape == AttackShape::Cone) {
        // 敵の距離が攻撃位置と完全に同じだったとき用
        if (distance <= 0.001f)
            return true;
        // 敵までの距離を正規化
        VECTOR enemyDir = VNorm(toTarget);
        // 正面が1背面が-1
        float dot = VDot(forward, enemyDir);
        // 扇形の角度範囲内なら当たる
        if (dot >= dotThreshold) {
            return true;
        }
    }
    if (shape == AttackShape::RectAngle) {
        float forwardDistance =
            toTarget.x * forward.x + toTarget.z * forward.z;

        float sideDistance =
            toTarget.x * forward.z - toTarget.z * forward.x;

        return forwardDistance >= 0.0f &&
            forwardDistance <= range &&
            fabsf(sideDistance) <= radius;
    }
    return false;
}

void DrawAttackArea(const AttackData& attack)
{
    const int segments = 32;

    VECTOR pos = attack.position;
    pos.y += 0.1f;
    auto GetPoint = [&](float angle) -> VECTOR {
        return VGet(
            pos.x + cosf(angle) * attack.range,
            pos.y,
            pos.z + sinf(angle) * attack.range
        );
    };

    switch (attack.shape) {
    case AttackShape::Cone:
    {
        // 内積のしきい値から片側の角度を求める
        float halfAngle = acosf(attack.dotThreshold);

        // 正面方向の角度
        float baseAngle = atan2f(
            attack.forward.z,
            attack.forward.x
        );

        // 扇形の開始角度と終了角度
        float startAngle = baseAngle - halfAngle;
        float endAngle = baseAngle + halfAngle;

        // 1回あたりの角度の変化量
        float step = (endAngle - startAngle) / segments;

        // 円弧を描画
        for (int i = 0; i < segments; i++) {
            float angle1 = startAngle + step * i;
            float angle2 = startAngle + step * (i + 1);

            DrawLine3D(
                GetPoint(angle1),
                GetPoint(angle2),
                RED
            );
        }

        // 扇形の左右の直線
        DrawLine3D(pos, GetPoint(startAngle), RED);
        DrawLine3D(pos, GetPoint(endAngle), RED);

        break;
    }

    case AttackShape::Circle: 
    {
        float step = 2.0f * DX_PI / segments;

        for (int i = 0; i < segments; i++) {
            float angle1 = step * i;
            float angle2 = step * (i + 1);

            DrawLine3D(
                GetPoint(angle1),
                GetPoint(angle2),
                RED
            );
        }
        break;
    }
        
    case AttackShape::RectAngle: 
    {
        // 横方向のベクトル
        VECTOR right = VGet(
            attack.forward.z,
            0.0f,
            -attack.forward.x
        );

        // 横方向の半幅
        VECTOR side = VScale(right, attack.radius);

        // 正面方向の長さ
        VECTOR front = VScale(
            attack.forward,
            attack.range
        );

        // 四角形の4つの頂点
        VECTOR p1 = VSub(pos, side);
        VECTOR p2 = VAdd(pos, side);
        VECTOR p3 = VAdd(p2, front);
        VECTOR p4 = VAdd(p1, front);

        // 四角形の枠を描画
        DrawLine3D(p1, p2, RED);
        DrawLine3D(p2, p3, RED);
        DrawLine3D(p3, p4, RED);
        DrawLine3D(p4, p1, RED);

        break;
    }

    }
}

#pragma region AttackData

AttackData WarriorSwingAttackArea = [] {
    AttackData attack;
    attack.shape = AttackShape::Cone;
    attack.moveType = AttackMoveType::None;
    attack.range = 100.0f;
    attack.dotThreshold = 0.5f;
    attack.power = 10;
    attack.coolDown = 300;
    attack.animationType = AttackAnimationType::Swing;
    attack.knockback = 5.0f;
    return attack;
    }();

AttackData WarriorSlamAttackArea = [] {
    AttackData attack;
    attack.shape = AttackShape::RectAngle;
    attack.range = 80.0f;
    attack.power = 10;
    attack.coolDown = 300;
    attack.animationType = AttackAnimationType::Slam;
    attack.radius = 30.0f;
    attack.stunFrames = 300;
    attack.knockback = 5.0f;
    return attack;
    }();


#pragma endregion