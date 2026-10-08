//Character.cpp
#include"Character.h"
#include"Input.h"
#include"Constant.h"
#include <fstream>
#include <sstream>
#include <string>
#include<math.h>
#include<queue>
#include <algorithm>
#include"MapLoader.h"
#include"GameObjects.h"
#include"FontManager.h"

//共通関数
void DrawQuad3D(VECTOR a, VECTOR b, VECTOR c, VECTOR d, int color) {
    DrawTriangle3D(a, b, c, color, true);
    DrawTriangle3D(a, c, d, color, true);
}

#pragma region === Player ===
Player::Player() :Character(10)
{
    Init();
}
Player::~Player() {
}

void Player::Init() {

    pos = GetStartPosition();
    y = 0;
    vy = 0;
    m_isGround = true;
    
    m_life = 3;

    angle = { 0.0f, 1.5f };

    m_layer = GetMapLayerFromWorldY(pos.y);
}

void Player::Update() {
    m_isMoving = false;
    if (m_life <= 0) {
        
    }
    else {
        Move();
        UpdateState();
        SetGameLoadedLayer(m_layer);
    }

}

void Player::Draw() {
    float swing = sinf(m_animTime) * 10.0f;

    const int bodyColor = GetColor(255, 0, 0);
    const int frontColor = GetColor(255, 255, 0);

    float legHalfWidth = m_size * 0.35f;
    float armHalfWidth = m_size * 0.35f;
    float legOffset = m_size * 0.55f;
    float armOffset = m_size + armHalfWidth;

    VECTOR leftArmOffset = VGet(-armOffset, m_size * 2, 0.0f);
    VECTOR rightArmOffset = VGet(armOffset, m_size * 2, 0.0f);
    VECTOR leftLegOffset = VGet(-legOffset, m_size, 0.0f);
    VECTOR rightLegOffset = VGet(legOffset, m_size, 0.0f);

    VECTOR leftEyeOffset = VGet(-m_size * 0.4f, m_size * 0.3f, m_size + 1.0f);
    VECTOR rightEyeOffset = VGet(m_size * 0.4f, m_size * 0.3f, m_size + 1.0f);

    VECTOR mouthOffset = VGet(0.0f, -m_size * 0.3f, m_size + 1.0f);

    if (m_isMoving) {
        m_animTime += 0.1f;
        ApplyWalkAnimation(leftArmOffset, rightArmOffset, leftLegOffset, rightLegOffset);
    }
    else {
        m_animTime = 0;
        ApplyIdolAnimation(leftArmOffset, rightArmOffset, leftLegOffset, rightLegOffset);
    }

    VECTOR head = VGet(pos.x, pos.y + m_size * 3 + m_size, pos.z);
    VECTOR leftArm = VAdd(pos, RotateXZ(leftArmOffset));
    VECTOR rightArm = VAdd(pos, RotateXZ(rightArmOffset));
    VECTOR body = VGet(pos.x, pos.y + m_size * 2, pos.z);
    VECTOR leftLeg = VAdd(pos, RotateXZ(leftLegOffset));
    VECTOR rightLeg = VAdd(pos, RotateXZ(rightLegOffset));

    VECTOR leftEye = VAdd(head, RotateXZ(leftEyeOffset));
    VECTOR rightEye = VAdd(head, RotateXZ(rightEyeOffset));
    VECTOR mouth = VAdd(head, RotateXZ(mouthOffset));

    DrawBox(head, m_size, m_size, m_size, frontColor);
    DrawBox(leftArm, legHalfWidth, m_size / 2, m_size, frontColor);
    DrawBox(rightArm, legHalfWidth, m_size / 2, m_size, frontColor);
    DrawBox(body, m_size, m_size, m_size, bodyColor);
    DrawBox(leftLeg, legHalfWidth, m_size/2, m_size, bodyColor);
    DrawBox(rightLeg, legHalfWidth, m_size/2, m_size, bodyColor);

    DrawBox(leftEye, m_size * 0.15f, 2.0f, m_size * 0.15f, BLACK);
    DrawBox(rightEye, m_size * 0.15f, 2.0f, m_size * 0.15f, BLACK);

    DrawBox(mouth, m_size * 0.2f, 1.0f, m_size * 0.3f, BLACK);

    VECTOR forward = VGet(
        sinf(angle.x),
        0.0f,
        cosf(angle.x)
    );

    VECTOR start = VGet(
        pos.x,
        pos.y + m_size * 2,
        pos.z
    );

    VECTOR end = VGet(
        start.x + forward.x * 100.0f,
        start.y,
        start.z + forward.z * 100.0f
    );

    DrawLine3D(
        start,
        end,
        GetColor(0, 255, 0)
    );
}

void Player::Move() {
    float x = Input::GetAxisLX();
    float y = Input::GetAxisLY();

    float moveX = 0;
    float moveZ = 0;

    moveX += sinf(C_angle.x) * y;
    moveZ += cosf(C_angle.x) * y;

    moveX += cosf(C_angle.x) * x;
    moveZ -= sinf(C_angle.x) * x;

    float length = sqrtf(moveX * moveX + moveZ * moveZ);
    if (length > 1.0f) {
        moveX /= length;
        moveZ /= length;
    }

    if (length > 0.1f) {
        angle.x = atan2f(moveX, moveZ);
    }

    float currentSpeed = speed;

    m_isMoving = length > 0.1f;

    if (Input::IsActionPressed(Action::Dash) && length > 0.1f) {
        currentSpeed *= 2.0f;
        m_isDashing = true;
    }
    else {
        m_isDashing = false;
    }

    const float playerRadius = 60.0f;
    VECTOR nextPos = pos;
    nextPos.x += moveX * currentSpeed;
    nextPos.z += moveZ * currentSpeed;
    pos = ResolvePlayerMapCollisionForPlayer(pos, nextPos, playerRadius, y, m_layer);
    // 階層変更は高さだけでは決めない。
    // 階段の上端/下端まで進んだ時だけ、TryMoveLayerByStairs が m_layer を切り替える。

}

void Player::ApplyIdolAnimation(VECTOR& leftArm, VECTOR& rightArm, VECTOR& leftLeg, VECTOR& rightLeg)
{
    
}

void Player::ApplyWalkAnimation(VECTOR& leftArm, VECTOR& rightArm, VECTOR& leftLeg, VECTOR& rightLeg)
{
    float swing = sinf(m_animTime) * 10.0f;

    leftArm.z -= swing;
    rightArm.z += swing;

    leftLeg.z += swing;
    rightLeg.z -= swing;
}

void Player::DrawBox(VECTOR center, float halfWidth, float halfDepth, float halfHeight, int bodyColor) {
    VECTOR bottom[4];
    VECTOR top[4];

    CreateDrawVertices(center, bottom, top, halfWidth, halfDepth, halfHeight);

    //下
    DrawQuad3D(bottom[0], bottom[1], bottom[2], bottom[3], bodyColor);
    //上
    DrawQuad3D(top[0], top[1], top[2], top[3], bodyColor);
    //側面
    DrawQuad3D(bottom[0], bottom[1], top[1], top[0], bodyColor);
    DrawQuad3D(bottom[0], bottom[3], top[3], top[0], bodyColor);
    DrawQuad3D(bottom[1], bottom[2], top[2], top[1], bodyColor);
    DrawQuad3D(bottom[2], bottom[3], top[3], top[2], bodyColor);
}

void Player::CreateDrawVertices(VECTOR center, VECTOR bottom[4], VECTOR top[4], float halfWidth, float halfDepth, float halfHeight) {
    VECTOR local[4] = {
        VGet(-halfWidth, 0.0f, -halfDepth),
        VGet(halfWidth, 0.0f, -halfDepth),
        VGet(halfWidth, 0.0f,  halfDepth),
        VGet(-halfWidth, 0.0f,  halfDepth)
    };

    for (int i = 0; i < 4; i++) {
        VECTOR rotated = RotateXZ(local[i]);

        bottom[i] = VGet(center.x + rotated.x, center.y - halfHeight, center.z + rotated.z);
        top[i] = VGet(center.x + rotated.x, center.y + halfHeight, center.z + rotated.z);
    }
}

VECTOR Player::RotateXZ(VECTOR v)
{
    float cosAngle = cosf(angle.x);
    float sinAngle = sinf(angle.x);

    return VGet(v.x * cosAngle + v.z * sinAngle, v.y, -v.x * sinAngle + v.z * cosAngle);
}

void Player::UpdateState() {
    float x = Input::GetAxisLX();
    float y = Input::GetAxisLY();
}

void Player::Damage() {
    // 2026-06-02: GameObjectsから受けたダメージを既存の死亡/リスポーン処理へ集約するため追加。
   // 2026-06-02: ギミックのダメージでライフが0未満へ減り続けないよう追加。
    if (m_life <= 0)
        return;

    m_life--;
    // 2026-06-15: ライフが残っている時は、STARTではなく有効チェックポイントへ復帰する。
    pos = GetGameRespawnPosition();
    SetGameLoadedLayer(m_layer);
    y = 0;
    vy = 0;
    m_isGround = true;
    m_layer = GetMapLayerFromWorldY(pos.y + BLOCK_SIZE * 0.5f);
}

bool Player::IsAlive() const {
    // 2026-06-02: ライフが尽きた時にScene側でリザルトへ遷移するため追加。
    return m_life > 0;
}

int Player::GetLife() const {
    // 2026-06-02: ゲーム画面のHUDに残りライフを表示するため追加。
    return m_life;
}

int Player::GetLayer() const {
    return m_layer;
}

void Player::SetCameraAngle(Angle& a) { C_angle = a; }
#pragma endregion
#pragma region === Camela ===
Camela::Camela(Player& p) :Character(0), p(p), distance(360),frontLight(0)
{
    Init();
}

void Camela::Init() {
    camelaAngle = { 0.0f, 0.3f };

    frontLight = CreateSpotLightHandle(
        VGet(0, 0, 0),        // ライト位置
        VGet(0, -1, 0),       // 向き
        DX_PI_F / 4.0f,       // 外側の角度
        DX_PI_F / 8.0f,       // 内側の角度
        900.0f,               // 距離
        0.8f,                 // 強さ
        0.002f,
        0.0f
    );
}

void Camela::Update() {
    p.SetCameraAngle(camelaAngle);
    VECTOR target = p.getVECTOR();
    target.y += 80.0f; // 少し上を見るなら足元ではなく体の中心あたり

    VECTOR idealCamPos = VGet(
        target.x - sinf(camelaAngle.x) * distance,
        target.y + sinf(camelaAngle.y) * distance,
        target.z - cosf(camelaAngle.x) * distance
    );

    const float cameraRadius = 20.0f;
    int layer = p.GetLayer();

    VECTOR camPos = ResolveCameraCollision(
        target,
        idealCamPos,
        cameraRadius,
        layer
    );

    VECTOR dir = VSub(target, camPos);
    dir = VNorm(dir);

    SetLightPositionHandle(frontLight, camPos);
    SetLightDirectionHandle(frontLight, dir);

    VECTOR lightPos = VGet(
        target.x,
        target.y + 500.0f,
        target.z
    );

    SetLightPosition(lightPos);

    
    SetLightDirection(VGet(0,-1.0f,0));
    

    SetCameraPositionAndTarget_UpVecY(camPos, target);

    MoveAngle();
}

void Camela::Draw() {

}

void Camela::MoveAngle() {
    float padRX = Input::GetAxisRX();
    float padRY = Input::GetAxisRY();

    float mouseDX = (float)Input::GetMouseDeltaX();
    float mouseDY = (float)Input::GetMouseDeltaY();

    if (Input::IsCamelaRightLeftFlip()) {
        padRX *= -1.0f;
        mouseDX *= -1.0f;
    }

    if (Input::IsCamelaUpDownFlip()) {
        padRY *= -1.0f;
        mouseDY *= -1.0f;
    }

    camelaAngle.x += padRX * Input::GetPadSensitivity();
    camelaAngle.y += padRY * Input::GetPadSensitivity();

    camelaAngle.x += mouseDX * Input::GetMouseSensitivity();
    camelaAngle.y += mouseDY * Input::GetMouseSensitivity();

    camelaAngle.y = Clamp(camelaAngle.y, 0.0f, 1.5f);
}
#pragma endregion
// 7/1追加
// 敵の処理(仮)
#pragma region === Enemy ===
Enemy::Enemy() : Character(7), isActive(false), m_playerPos({0,0}), m_playerLayer(0), m_id(0) {}

void Enemy::Init(int y, int x, int z, int id) {
    m_layer = y;
    m_id = id;
    const float enemyRadius = 10.0f;

    pos = VGet(
        CellToWorldCenter(x),
        y * BLOCK_SIZE,
        CellToWorldCenter(z)
    );

    switch (id) {
    case 0:
        break;
    case 1:
        break;
    }

    // 2026-07-11: エディターで追加した巡回ポイントがある場合は、そのルートを優先して使う。
    patrolPoints.clear();
    int patrolCount = EnemyPatrolCountMap[y][z][x];
    for (int i = 0; i < patrolCount; i++)
    {
        int px = EnemyPatrolXMap[y][z][x][i];
        int pz = EnemyPatrolZMap[y][z][x][i];
        if (px >= 0 && pz >= 0)
            patrolPoints.push_back({ px, pz });
    }
    if (patrolPoints.empty())
    {
        patrolPoints.push_back({ x + 3, z });
        patrolPoints.push_back({ x + 3, z + 3 });
        patrolPoints.push_back({ x, z + 3 });
        patrolPoints.push_back({ x, z });
    }

    isActive = true;
}

void Enemy::Update() {
    Move();
}

void Enemy::Draw() {
    DrawLayer(m_layer);
}

void Enemy::DrawLayer(int drawLayer) {
    if (!isActive)
        return;

    // 2026-07-16: 別階層の敵が壁越しに見えないよう、現在描画中の階層だけ表示する。
    if (m_layer != drawLayer)
        return;
}

void Enemy::Move() {
    if (!isActive)
        return;

    float dx = m_playerPos.x - pos.x;
    float dz = m_playerPos.z - pos.z;
    float distSq = dx * dx + dz * dz;

    if (m_playerLayer == m_layer && distSq < chaseRange * chaseRange)
    {
        state = EnemyState::Chase;
    }
    else
    {
        state = EnemyState::Patrol;
    }

    if (state == EnemyState::Chase)
    {
        MoveToPlayer();
    }
    else
    {
        MovePatrol();
    }
}

void Enemy::MoveWithHalfStep(VECTOR nextPos)
{
    const float enemyRadius = 10.0f;

    float currentGroundY =
        GetMapGroundY(
            pos.x,
            pos.z,
            pos.y,
            enemyRadius
        );

    float nextGroundY =
        GetMapGroundY(
            nextPos.x,
            nextPos.z,
            pos.y + BLOCK_SIZE * 0.5f,
            enemyRadius
        );

    float diff = nextGroundY - currentGroundY;

    // 上れる高さかどうか
    bool canStepUp = diff > 0.0f && diff <= BLOCK_SIZE * 0.5f + 5.0f;

    VECTOR currentPos = pos;

    if (canStepUp)
    {
        // 現在位置と移動先の両方を足場の高さに合わせる
        currentPos.y = nextGroundY;
        nextPos.y = nextGroundY;
    }

    VECTOR resolvedPos = ResolvePlayerMapCollision(
        currentPos,
        nextPos,
        enemyRadius,
        currentPos.y,
        m_layer
    );

    pos.x = resolvedPos.x;
    pos.z = resolvedPos.z;

    float groundY = GetMapGroundY(
        pos.x,
        pos.z,
        pos.y + BLOCK_SIZE * 0.5f,
        enemyRadius
    );

    if (canStepUp)
    {
        // 0.5足場に乗るときは確実に高さを合わせる
        pos.y = nextGroundY;
    }
    else
    {
        // 通常床や下りでは滑らかに高さを合わせる
        pos.y += (groundY - pos.y) * 0.2f;
    }
}

void Enemy::MoveToPlayer(){
    if (!isActive)
        return;

    int enemyX = WorldToCell(pos.x);
    int enemyZ = WorldToCell(pos.z);

    if (m_playerLayer != m_layer)
        return;

    int playerX = WorldToCell(m_playerPos.x);
    int playerZ = WorldToCell(m_playerPos.z);

    pathTimer++;

    // 毎フレームBFSすると重いので、少し間隔を空ける
    if (pathTimer >= 30)
    {
        path = FindPathBFS(m_layer, enemyX, enemyZ, playerX, playerZ);
        pathTimer = 0;
    }

    if (path.size() < 2)
        return;

    Pos2 next = path[1];

    VECTOR target = VGet(
        CellToWorldCenter(next.x),
        pos.y,
        CellToWorldCenter(next.z)
    );

    VECTOR dir = VSub(target, pos);
    dir.y = 0.0f;

    float len = VSize(dir);

    if (len < speed)
    {
        VECTOR nextPos = pos;
        nextPos.x = target.x;
        nextPos.z = target.z;

        // 直接posへ代入せず、必ず当たり判定を通す
        MoveWithHalfStep(nextPos);

        if (path.size() >= 2)
        {
            path.erase(path.begin());
        }

        return;
    }

    dir = VNorm(dir);

    VECTOR nextPos = pos;
    nextPos.x += dir.x * speed;
    nextPos.z += dir.z * speed;

    MoveWithHalfStep(nextPos);
}

void Enemy::MovePatrol() {
    if (patrolPoints.empty())
        return;

    int enemyX = WorldToCell(pos.x);
    int enemyZ = WorldToCell(pos.z);

    Pos2 goal = patrolPoints[patrolIndex];

    if (enemyX == goal.x && enemyZ == goal.z)
    {
        patrolIndex++;

        if (patrolIndex >= patrolPoints.size())
        {
            patrolIndex = 0;
        }

        path.clear();
        pathTimer = 30;
        return;
    }


    pathTimer++;

    if (pathTimer >= 30)
    {
        path = FindPathBFS(m_layer, enemyX, enemyZ, goal.x, goal.z);
        pathTimer = 0;
    }

    if (path.size() < 2)
        return;

    Pos2 next = path[1];

    MoveToCell(next);
}

void Enemy::MoveToCell(Pos2 next)
{
    VECTOR target = VGet(
        CellToWorldCenter(next.x),
        pos.y,
        CellToWorldCenter(next.z)
    );

    VECTOR dir = VSub(target, pos);
    dir.y = 0.0f;

    float len = VSize(dir);

    if (len < speed)
    {
        VECTOR nextPos = pos;
        nextPos.x = target.x;
        nextPos.z = target.z;

        MoveWithHalfStep(nextPos);

        if (path.size() >= 2)
        {
            path.erase(path.begin());
        }

        return;
    }

    dir = VNorm(dir);

    VECTOR nextPos = pos;
    nextPos.x += dir.x * speed;
    nextPos.z += dir.z * speed;

    MoveWithHalfStep(nextPos);

    float angle = atan2f(dir.x, dir.z);
}

bool Enemy::IsHitPlayer(VECTOR playerPos, int playerLayer) const
{
    if (!isActive)
        return false;

    if (playerLayer != m_layer)
        return false;

    float dx = playerPos.x - pos.x;
    float dz = playerPos.z - pos.z;
    float distSq = dx * dx + dz * dz;

    const float enemyRadius = 10.0f;
    const float playerRadius = 60.0f;
    const float hitRange = enemyRadius + playerRadius;

    return distSq <= hitRange * hitRange;
}

vector<Pos2> FindPathBFS(int layer, int startX, int startZ, int goalX, int goalZ)
{
    queue<Pos2> q;

    bool visited[BLOCK_NUM_Z][BLOCK_NUM_X] = {};
    Pos2 prev[BLOCK_NUM_Z][BLOCK_NUM_X];

    q.push({ startX, startZ });
    visited[startZ][startX] = true;

    int dx[4] = { 1, -1, 0, 0 };
    int dz[4] = { 0, 0, 1, -1 };

    while (!q.empty())
    {
        Pos2 current = q.front();
        q.pop();

        if (current.x == goalX && current.z == goalZ)
            break;

        for (int i = 0; i < 4; i++)
        {
            int nx = current.x + dx[i];
            int nz = current.z + dz[i];

            if (nx < 0 || nx >= BLOCK_NUM_X)
                continue;

            if (nz < 0 || nz >= BLOCK_NUM_Z)
                continue;

            if (visited[nz][nx])
                continue;

            if (!CanMoveCellToCell(
                layer,
                current.x,
                current.z,
                nx,
                nz))
            {
                continue;
            }

            visited[nz][nx] = true;
            prev[nz][nx] = current;

            q.push({ nx,nz });
        }
    }

    std::vector<Pos2> path;

    if (!visited[goalZ][goalX])
        return path;

    Pos2 current = { goalX, goalZ };

    while (!(current.x == startX && current.z == startZ))
    {
        path.push_back(current);
        current = prev[current.z][current.x];
    }

    path.push_back({ startX, startZ });
    std::reverse(path.begin(), path.end());

    return path;
}

vector<MapNode> FindMapPathBFS(
    int startLayer,
    int startX,
    int startZ,
    int goalLayer,
    int goalX,
    int goalZ)
{
    queue<MapNode> q;

    const int totalSize =
        BLOCK_NUM_Y *
        BLOCK_NUM_Z *
        BLOCK_NUM_X;

    // ヒープ領域に確保する
    vector<bool> visited(totalSize, false);
    vector<MapNode> prev(totalSize);

    auto GetIndex = [](int layer, int z, int x)
        {
            return
                layer * BLOCK_NUM_Z * BLOCK_NUM_X +
                z * BLOCK_NUM_X +
                x;
        };

    MapNode start = {
        startLayer,
        startX,
        startZ
    };

    MapNode goal = {
        goalLayer,
        goalX,
        goalZ
    };

    if (!IsMapPosValid(
        start.layer,
        start.z,
        start.x))
    {
        return {};
    }

    if (!IsMapPosValid(
        goal.layer,
        goal.z,
        goal.x))
    {
        return {};
    }

    q.push(start);

    visited[GetIndex(
        start.layer,
        start.z,
        start.x)] = true;

    const int dx[4] = {
        0, 1, 0, -1
    };

    const int dz[4] = {
        -1, 0, 1, 0
    };

    while (!q.empty())
    {
        MapNode current = q.front();
        q.pop();

        if (current == goal)
            break;

        // 同じ階層の上下左右
        for (int direction = 0;
            direction < 4;
            direction++)
        {
            int nextX =
                current.x + dx[direction];

            int nextZ =
                current.z + dz[direction];

            int nextLayer =
                current.layer;

            if (!IsMapPosValid(
                nextLayer,
                nextZ,
                nextX))
            {
                continue;
            }

            int nextIndex =
                GetIndex(nextLayer, nextZ, nextX);

            if (visited[nextIndex])
                continue;

            if (!CanMoveCellToCell(
                current.layer,
                current.x,
                current.z,
                nextX,
                nextZ))
            {
                continue;
            }

            MapNode next = {
                nextLayer,
                nextX,
                nextZ
            };

            visited[nextIndex] = true;
            prev[nextIndex] = current;

            q.push(next);
        }

        // 階層移動
        for (int direction = 0;
            direction < 4;
            direction++)
        {
            MapNode next;

            if (!GetLayerMoveDestination(
                current.layer,
                current.x,
                current.z,
                direction,
                next))
            {
                continue;
            }

            int nextIndex =
                GetIndex(
                    next.layer,
                    next.z,
                    next.x
                );

            if (visited[nextIndex])
                continue;

            visited[nextIndex] = true;
            prev[nextIndex] = current;

            q.push(next);
        }
    }

    vector<MapNode> path;

    int goalIndex =
        GetIndex(
            goal.layer,
            goal.z,
            goal.x
        );

    if (!visited[goalIndex])
        return path;

    MapNode current = goal;

    while (!(current == start))
    {
        path.push_back(current);

        int currentIndex =
            GetIndex(
                current.layer,
                current.z,
                current.x
            );

        current = prev[currentIndex];
    }

    path.push_back(start);

    reverse(
        path.begin(),
        path.end()
    );

    return path;
}

void Enemy::SetPlayerPos(VECTOR playerPos) {
    m_playerPos = playerPos;
}

void Enemy::SetPlayerLayer(int playerLayer) {
    m_playerLayer = playerLayer;
}
#pragma endregion
#pragma region === EnemyManager ===
EnemyManager::EnemyManager() {
    enemies.reserve(1000);
    pool.reserve(1000);
}

void EnemyManager::Init() {

    enemies.clear();
    pool.clear();

    // 2026-07-16: LoadGameMapが読み込んだmaps/<名前>/map.csvの敵配置から生成する。
    for (int y = 0; y < BLOCK_NUM_Y; y++)
    {
        for (int z = 0; z < BLOCK_NUM_Z; z++)
        {
            for (int x = 0; x < BLOCK_NUM_X; x++)
            {
                int id = GameEnemyMap[y][z][x];
                if (id < 0)
                    continue;

                Enemy enemy;
                enemy.Init(y, x, z, id);
                enemies.push_back(enemy);
            }
        }
    }
}

void EnemyManager::Update(VECTOR playerPos, int playerLayer) {
    for (auto& e : enemies) {
        e.SetPlayerPos(playerPos);
        e.SetPlayerLayer(playerLayer);

        // 現在表示している階層の敵だけ動かす
        if (e.GetLayer() == playerLayer)
        {
            e.Update();
        }
    }
}

void EnemyManager::Draw() {
    DrawLayer(GetGameDrawLayer());
}

void EnemyManager::DrawLayer(int drawLayer) {
    // 2026-07-23: 敵デバッグ文字は出さず、敵本体だけ描画する。
    for (auto& e : enemies)
    {
        e.DrawLayer(drawLayer);
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
    outLayer = enemy.GetLayer();
    return true;
}

bool EnemyManager::IsHitPlayer(VECTOR playerPos, int playerLayer) const
{
    for (const auto& enemy : enemies)
    {
        if (enemy.IsHitPlayer(playerPos, playerLayer))
        {
            return true;
        }
    }

    return false;
}
#pragma endregion
#pragma region === Item ===
void Item::Init(VECTOR pos, ItemType type) {

}
#pragma endregion

#pragma region === ItemManager ===

#pragma endregion


