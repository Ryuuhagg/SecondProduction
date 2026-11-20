#pragma once
#include"Character.h"
#include"Constant.h"
#include"Timer.h"
#include"AttackData.h"
enum class EnemyState {
	Move,	// 移動
    Attack, // 攻撃
};

class Enemy : public Character {
protected:
	//生存フラグ
	bool m_isActive;
	//ステータス
	int m_attack = 0;

    Damageable* m_target = nullptr;
    Cooldown m_coolDown;

    float m_radius;
    int m_stanFlame = 0;
    float m_knockback = 0;
    VECTOR m_knockbackDir = {0,0,0};
    int m_knockbackTime = 0;

    VECTOR m_targetPos;

    static constexpr int attackAnimationFrames = 12;
    int m_attackAnimationRemaining = 0;
public:
    Enemy(float speed, int maxHp);
    virtual ~Enemy() = default;

    virtual void Init(VECTOR pos) = 0;

    void Update() override;
    void Draw() override;

    void ApplyHit(const AttackData& data);

    virtual void Move() = 0;
    virtual void Attack() = 0;

    void SetTargetPosition(VECTOR targetPos);

    void SetTarget(Damageable *target);

    VECTOR GetPosition() const { return pos; }
    float GetRadius() const { return m_radius; }
    bool IsActive() const { return m_isActive; }
};

class EnemyMelee : public Enemy
{
public:
    EnemyMelee();

    void Init(VECTOR staerPos) override;

    void Move() override;
    void Attack() override;
};