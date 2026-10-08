#include"EnemyManager.h"
#pragma region === EnemyManager ===
EnemyManager::EnemyManager() {
    enemies.reserve(1000);
    pool.reserve(1000);
}

void EnemyManager::Init() {

    enemies.clear();
    pool.clear();

    
}

void EnemyManager::Update(VECTOR playerPos, int playerLayer) {
    for (auto& e : enemies) {
        e.Update();
    }
}

void EnemyManager::Draw() {
    
}

void EnemyManager::DrawLayer(int drawLayer) {
    // 2026-07-23: 敵デバッグ文字は出さず、敵本体だけ描画する。
    for (auto& e : enemies)
    {
        e.Draw();
    }
}

int EnemyManager::GetEnemyCount() const
{
    return (int)enemies.size();
}

bool EnemyManager::GetEnemyPosition(int index, VECTOR& outPos, int& outLayer) const
{
    if (index < 0 || index >= (int)enemies.size())
        return false;

    const Enemy& enemy = enemies[index];
    if (!enemy.IsActive())
        return false;

    outPos = enemy.GetPosition();
    return true;
}

bool EnemyManager::IsHitPlayer(VECTOR playerPos, int playerLayer) const
{
    for (const auto& enemy : enemies)
    {

    }

    return false;
}
#pragma endregion