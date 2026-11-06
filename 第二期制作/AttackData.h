#pragma once
#include"DxLib.h"
enum class AttackShape
{
    Cone,       // ‘O•ûîŒ`
    Circle      // üˆÍ
};

enum class AttackMoveType {
    None,       // ‚»‚Ìê‚ÅUŒ‚”»’è
    Straight    // ‚Ü‚Á‚·‚®”ò‚Ô
};

enum class AttackAnimationType {
    Swing,      // ‰¡U‚è
    Slam,       // ’@‚«‚Â‚¯
    Thrust,     // “Ë‚«
    Cast        // –‚–@
};

struct AttackData
{
    AttackShape shape;
    AttackMoveType moveType;
    VECTOR position;
    VECTOR forward;

    float range;
    float dotThreshold;

    int power;
    int coolDown;

    AttackAnimationType animationType;

    float moveSpeed;
    float radius;

    bool Contains(VECTOR targetPos) const;
};

const AttackData WarriorSwingAttackArea{
    AttackShape::Cone,
    AttackMoveType::None,
    VGet(0, 0, 0),
    VGet(0, 0, 0),
    100.0f,
    0.5f,
    10,
    300,
    AttackAnimationType::Swing,
    0.0f,   
    0.0f,   
};

const AttackData WarriorSlamAttackArea{
    AttackShape::Cone,
    AttackMoveType::None,
    VGet(0, 0, 0),
    VGet(0, 0, 0),
    100.0f,
    0.5f,
    10,
    300,
    AttackAnimationType::Slam,
    0.0f,   
    0.0f, 
};
