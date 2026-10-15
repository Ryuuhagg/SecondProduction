#include"EnemyManager.h"
#include"MapLoader.h"
#pragma region === EnemyManager ===
EnemyManager::EnemyManager() {
    enemies.reserve(1000);
    pool.reserve(1000);
}

void EnemyManager::Init() {

    enemies.clear();
    pool.clear();

    VECTOR start = GetStartPosition();

    SpawnMelee(VAdd(start, VGet(200.0f, 0.0f, 0.0f)));
    SpawnMelee(VAdd(start, VGet(-200.0f, 0.0f, 0.0f)));
    SpawnMelee(VAdd(start, VGet(0.0f, 0.0f, 200.0f)));
}

void EnemyManager::Update(VECTOR playerPos, Damageable *player) {

    for (auto& e : enemies) {
        e->SetTarget(player);
        e->SetTargetPosition(playerPos);
        e->Update();
    }
    ResolveEnemyCollision();
}

void EnemyManager::Draw() {
    for (auto& enemy : enemies){
        enemy->Draw();
    }
}
// 敵同士が重なっていたら離す関数
void EnemyManager::ResolveEnemyCollision() {
    // 2重ループで総当たり
    for (int i = 0; i < enemies.size(); i++) {
        for (int j = i + 1; j < enemies.size(); j++) {
            // 2種類の敵を取り出す
            Enemy* enemyA = enemies[i].get();
            Enemy* enemyB = enemies[j].get();
            // どっちかが倒れていたら次の組み合わせに
            if (!enemyA->IsActive() || !enemyB->IsActive())
                continue;
            // 敵Aから敵Bまでのベクトルを計算
            VECTOR diff = VSub(enemyB->GetPosition(),enemyA->GetPosition());
            // 今回はY座標を使わないから0にしておく
            diff.y = 0.0f;
            // ベクトルの長さを求める
            float distance = VSize(diff);
            // 敵が離れていてほしい長さ（今回は敵の半径が20.0fなので半径の2倍の大きさにした）
            const float minDistance = enemyA[i].GetRadius() + enemyB[j].GetRadius();
            // 敵が重なっているか？
            if (distance < minDistance) {
                // どのくらい重なっているか
                float overlap = minDistance - distance;
                VECTOR direction;
                if (distance <= 0.001f) {
                    // 同じ位置だった時
                    direction = VGet(1.0f, 0.0f, 0.0f);
                }
                else {
                    // diff（敵Ａから敵Ｂまでのベクトル）を正規化
                    direction = VNorm(diff);
                }
                // 押し出される大きさ
                VECTOR push = VScale(direction, overlap * 0.5f);
                // 現在位置から押し出される大きさ分移動
                // diffがA->BのベクトルだからAは反対側にする
                enemyA->AddPosition(VScale(push, -1.0f));
                enemyB->AddPosition(push);
            }
        }
    }
}

void EnemyManager::AttackEnemies(const AttackArea& attackArea)
{
    for (auto& e : enemies)
    {
        if (!e->IsActive())
            continue;

        if (attackArea.Contains(e->GetPosition()))
        {
            e->Damage(attackArea.damage);

            printfDx("Enemy Hit\n");
            printfDx("Enemy Hp : %d\n", e->GetHp());
        }
    }
}

void EnemyManager::SpawnMelee(VECTOR pos)
{
    auto enemy = make_unique<EnemyMelee>();

    enemy->Init(pos);

    enemies.push_back(move(enemy));
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