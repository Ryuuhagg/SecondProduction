#pragma once
#include"Constant.h"
class Weapon {
protected:
	float m_length;
	float m_halfWidth;
public:
    Weapon(float length, float halfWidth)
        : m_length(length), m_halfWidth(halfWidth) {
    }

    float GetLength() const { return m_length; }
    float GetHalfWidth() const { return m_halfWidth; }

    // direction は手元から棒の先に向かう、長さ1のベクトル
    VECTOR GetTip(VECTOR grip, VECTOR direction) const {
        return VAdd(grip, VScale(direction, m_length));
    }
};

class Sword : public Weapon {
public:
    Sword(float length, float halfWidth)
        : Weapon(length, halfWidth) {
    }
};