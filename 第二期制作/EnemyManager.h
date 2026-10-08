#pragma once
#include"Character.h"
#include"Enemy.h"
class EnemyManager {
    vector<Enemy> enemies;
    vector<int> pool;
public:
    EnemyManager();
    void Init();
    void Update(VECTOR playerPos, int playerLayer);
    void Draw();
    void DrawLayer(int drawLayer);
    int GetEnemyCount() const;
    bool GetEnemyPosition(int index, VECTOR& pos, int& layer) const;

    bool IsHitPlayer(VECTOR playerPos, int playerLayer) const;
};

extern EnemyManager e;