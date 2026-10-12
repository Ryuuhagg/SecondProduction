#include"Enemy.h"

#pragma region === Enemy ===
Enemy::Enemy(float speed, int maxHp) : Character(speed, maxHp), m_isActive(false) ,m_coolDown(1), m_targetPos(VGet(0,0,0)), m_target(nullptr) {}

void Enemy::Init(VECTOR pos) {
    const float enemyRadius = 10.0f;
    m_isActive = true;
}

void Enemy::Update() {
    if (!m_isActive)
        return;

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

void Enemy::SetTargetPosition(VECTOR targetPos)
{
    m_targetPos = targetPos;
}

void Enemy::SetTarget(Damageable* target) {
    m_target = target;
}

#pragma endregion

#pragma region MeleeEnemy
EnemyMelee::EnemyMelee():Enemy(1,100){}

void EnemyMelee::Init(VECTOR startPos){
    pos = startPos;

    m_hp = m_maxHp;
    m_attack = 10;
    m_isActive = true;

    m_coolDown.SetInterval(1000);
}

void EnemyMelee::Attack() {
    if (m_target == nullptr)
        return;

    m_target->Damage(m_attack);
}

void EnemyMelee::Move() {
    VECTOR dir = VSub(m_targetPos, pos);
    dir.y = 0.0f;

    float length = VSize(dir);

    // UŒ‚‰Â”\‹——£‚Ü‚Å‹ß‚Ã‚¢‚½‚ç‚»‚êˆÈã‹ß‚Ã‚©‚È‚¢
    const float attackRange = 80.0f;
    if (length <= attackRange) {
        if (m_coolDown.IsReady()) {
            Attack();
            m_coolDown.Reset();
        }
        return;
    }

    dir = VNorm(dir);

    pos.x += dir.x * m_speed;
    pos.z += dir.z * m_speed;
}


#pragma endregion