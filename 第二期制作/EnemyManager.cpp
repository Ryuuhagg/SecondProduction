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

void EnemyManager::Update() {
    for (auto& e : enemies) {
        e->Update();
    }
}

void EnemyManager::Draw() {
    for (auto& enemy : enemies){
        enemy->Draw();
    }
}

void EnemyManager::SpawnMelee(VECTOR pos)
{
    auto enemy = std::make_unique<EnemyMelee>();

    enemy->Init(pos);

    enemies.push_back(std::move(enemy));
}

int EnemyManager::GetEnemyCount() const
{
    return (int)enemies.size();
}

bool EnemyManager::GetEnemyPosition(int index, VECTOR& outPos) const
{
    if (index < 0 || index >= (int)enemies.size())
        return false;

    const Enemy* enemy = enemies[index].get();

    if (!enemy->IsActive())
        return false;

    outPos = enemy->GetPosition();

    return true;
}

#pragma endregion