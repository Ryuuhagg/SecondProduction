#include"Enemy.h"

#pragma region === Enemy ===
Enemy::Enemy(float speed) : Character(speed), m_isActive(false) {}

void Enemy::Init(VECTOR pos) {
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

void Enemy::Damage(int damage) {

}

#pragma endregion

#pragma region MeleeEnemy
EnemyMelee::EnemyMelee():Enemy(10){}


void EnemyMelee::Init(VECTOR pos) {

}

void EnemyMelee::Attack() {

}

void EnemyMelee::Move() {

}


#pragma endregion