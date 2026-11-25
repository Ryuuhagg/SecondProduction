#pragma once
#include"DxLib.h"
enum class AttackShape
{
    Cone,       // 前方扇形
    Circle,     // 周囲
    RectAngle,  // 前方長方形
};

enum class AttackMoveType {
    None,       // その場で攻撃判定
    Straight    // まっすぐ飛ぶ
};

enum class AttackAnimationType {
    Swing,      // 横振り
    Slam,       // 叩きつけ
    Thrust,     // 突き
    Cast        // 魔法
};

struct AttackData
{
    AttackShape shape = AttackShape::Cone;
    AttackMoveType moveType = AttackMoveType::None;
    VECTOR position = VGet(0, 0, 0);
    VECTOR forward = VGet(0, 0, 0);

    float range = 0.0f;
    float dotThreshold = 0.0f;

    int power = 0;
    int coolDown = 0;

    AttackAnimationType animationType = AttackAnimationType::Swing;

    float moveSpeed = 0.0f;
    float radius = 0.0f;

    float knockback = 0.0f;
    int stunFrames = 0;

    bool Contains(VECTOR targetPos) const;
};

void DrawAttackArea(const AttackData& attack);

extern AttackData WarriorSwingAttackArea;
extern AttackData WarriorSlamAttackArea;