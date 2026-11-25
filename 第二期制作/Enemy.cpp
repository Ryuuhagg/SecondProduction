#include"Enemy.h"

#pragma region === Enemy ===
Enemy::Enemy(float speed, int maxHp) : Character(speed, maxHp), m_isActive(false) ,m_coolDown(1),m_radius(10), m_targetPos(VGet(0,0,0)), m_target(nullptr) {}
using namespace EnemyTuning;
void Enemy::Init(VECTOR pos) {
    m_isActive = true;
}

void Enemy::Update() {
    if (!m_isActive)
        return;

    if (!IsAlive()) {
        m_isActive = false;
        printfDx("Enemy Die\n");
        return;
    }

    if (m_attackAnimationRemaining > 0) {
        --m_attackAnimationRemaining;
    }
    if (m_knockbackTime > 0)
    {
        pos = VAdd(pos, VScale(m_knockbackDir, m_knockback));
        --m_knockbackTime;
    }
    else if (m_stanFlame > 0)
    {
        --m_stanFlame;
    }
    else
    {
        Move();
    }
}

void Enemy::Draw() {
    if (!m_isActive)
        return;

    const float attackAmount =
        static_cast<float>(m_attackAnimationRemaining) / attackAnimationFrames;

    const float drawRadius = m_radius * (1.0f + 0.25f * attackAmount);
    const int green = static_cast<int>(255.0f * attackAmount);

    DrawSphere3D(
        pos,
        drawRadius,
        16,
        GetColor(255, green, 0),
        GetColor(255, 255, 255),
        TRUE
    );
}

void Enemy::ApplyHit(const AttackData& data) {
    Damage(data.power);

    m_stanFlame = data.stunFrames;
    m_knockback = data.knockback;
    m_knockbackTime = knockbackTime;

    m_knockbackDir = VSub(pos, data.position);
    m_knockbackDir.y = 0.0f;

    if (VSize(m_knockbackDir) > 0.001f)
        m_knockbackDir = VNorm(m_knockbackDir);
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
    m_radius = 20;

    m_coolDown.SetInterval(1000);
}

void EnemyMelee::Attack() {
    if (m_target == nullptr)
        return;

    m_target->Damage(m_attack);
    m_attackAnimationRemaining = attackAnimationFrames;
}

void EnemyMelee::Move() {
    VECTOR dir = VSub(m_targetPos, pos);
    dir.y = 0.0f;

    float length = VSize(dir);

    // çUåÇâ¬î\ãóó£Ç‹Ç≈ãﬂÇ√Ç¢ÇΩÇÁÇªÇÍà»è„ãﬂÇ√Ç©Ç»Ç¢
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