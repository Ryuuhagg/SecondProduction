#pragma once
#include"Character.h"
#include"Enemy.h"
#include<memory>
class EnemyManager {
    vector<unique_ptr<Enemy>> enemies;
    vector<int> pool;
public:
    EnemyManager();

    void Init();
    void Update(VECTOR playerPos, Damageable *player);
    void Draw();

    void ResolveEnemyCollision();

    void AttackEnemies(VECTOR attackPos, VECTOR forward, float attackRange, int damage);
    void SpawnMelee(VECTOR pos);

    int GetEnemyCount() const;

    bool GetEnemyPosition(int index, VECTOR& pos) const;
};

extern EnemyManager e;