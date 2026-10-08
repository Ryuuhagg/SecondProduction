#pragma once
#include"Character.h"
#include"Constant.h"
enum class EnemyState {
	Move,	// 移動
};

class Enemy : public Character {
protected:
	//生存フラグ
	bool m_isActive;
	//ステータス
	int m_hp = 0;
	int m_attack = 0;
public:
    Enemy(float speed);
    virtual ~Enemy() = default;

    virtual void Init(VECTOR pos) = 0;

    void Update() override;
    void Draw() override;

    virtual void Move() = 0;
    virtual void Attack() = 0;

    void Damage(int damage);

    bool IsActive() const { return m_isActive; }
    bool IsAlive() const { return m_hp > 0; }
};

class EnemyMelee : public Enemy
{
public:
    EnemyMelee();

    void Init(VECTOR pos) override;

    void Move() override;
    void Attack() override;
};