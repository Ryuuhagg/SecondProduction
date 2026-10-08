#include"Enemy.h"

#pragma region === Enemy ===
Enemy::Enemy() : Character(7), m_isActive(false) {}

void Enemy::Init(int y, int x, int z, int id) {
    const float enemyRadius = 10.0f;
    m_isActive = true;
}

void Enemy::Update() {
    Move();
}

void Enemy::Draw() {
    if (!m_isActive)
        return;

    DrawSphere3D(
        pos,
        20.0f,
        16,
        GetColor(255, 0, 0),
        GetColor(255, 255, 255),
        TRUE
    );
}

void Enemy::Move() {
    if (!m_isActive)
        return;

}

#pragma endregion