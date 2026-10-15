#pragma once
#include"DxLib.h"
enum class AttackShape
{
    Cone,       // ‘O•ûîŒ`
    Circle      // üˆÍ
};

struct AttackArea
{
    AttackShape shape;

    VECTOR position;
    VECTOR forward;

    float range;
    float dotThreshold;

    int damage;

    bool Contains(VECTOR targetPos) const;
};

