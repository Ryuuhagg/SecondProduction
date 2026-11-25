//constant.h
#pragma once
#include"DxLib.h"

const int WIDTH = 1280;
const int HEIGHT = 720;

const int RED = GetColor(255, 0, 0);
const int BLUE = GetColor(0, 255, 0);
const int GREEN = GetColor(0, 0, 255);
const int WHITE = GetColor(255, 255, 255);
const int BLACK = GetColor(0, 0, 0);


constexpr float default_size = 10.0f;

namespace PlayerTuning {

    constexpr int SwingAttackTime = 180;
    constexpr int SlamAttackTime = 120;
}

namespace EnemyTuning {
    constexpr int knockbackTime = 10;
}

namespace WeaponData {
    constexpr float swordHalfWidth = default_size * 0.12f;
    const float swordHalfLength = default_size * 1.1f;
    const float swordCenterRadius = default_size * 2.8f;
    const float swordLength = 66.0f;
}


template <typename T>
T Clamp(T v, T min, T max) {
    if (v < min) return min;
    if (v > max) return max;
    return v;
}
// 7/17 BFSようの座標
struct MapNode
{
    int layer;
    int x;
    int z;

    bool operator==(const MapNode& other) const
    {
        return
            layer == other.layer &&
            x == other.x &&
            z == other.z;
    }
};