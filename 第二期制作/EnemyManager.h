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
    void Update();
    void Draw();

    void SpawnMelee(VECTOR pos);

    int GetEnemyCount() const;

    bool GetEnemyPosition(int index, VECTOR& pos) const;
};

extern EnemyManager e;