#include"Player.h"
#include"MapLoader.h"
#include"Input.h"
#include"GameObjects.h"
#pragma region === Player ===
Player::Player() :Character(10)
{
    Init();
}
Player::~Player() {
}

void Player::Init() {

    pos = GetStartPosition();

    m_life = 3;

    angle = { 0.0f, 1.5f };
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
    DrawBox(leftLeg, legHalfWidth, m_size / 2, m_size, bodyColor);
    DrawBox(rightLeg, legHalfWidth, m_size / 2, m_size, bodyColor);

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