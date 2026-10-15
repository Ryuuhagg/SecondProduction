#include"Player.h"
#include"MapLoader.h"
#include"Input.h"
#include"GameObjects.h"
#pragma region === Player ===
Player::Player() :Character(10, 10000), m_coolDown(1000)
{
    Init();
}
Player::~Player() {
}

void Player::Init() {

    pos = GetStartPosition();

    angle = { 0.0f, 1.5f };
    m_coolDown.SetInterval(1000);
}

void Player::Update() {
    m_attackTriggered = false;
    m_isMoving = false;
    
    Move();
    UpdateState();
    SetGameLoadedLayer(m_layer);

    if (Input::IsActionTrigger(Action::Attack)) {
        if (m_coolDown.IsReady()) {
            NormalAttack();
            m_coolDown.Reset();
        }
    }

    if (m_isAttacking && m_attackTimer.IsFinished()) {
        m_isAttacking = false;
        m_attackTimer.Stop();
    }

}

void Player::Draw() {
    const int bodyColor = GetColor(255, 0, 0);
    const int frontColor = GetColor(255, 255, 0);

    float legHalfWidth = m_size * 0.35f;
    float armHalfWidth = m_size * 0.35f;
    float legOffset = m_size * 0.55f;
    float armOffset = m_size + armHalfWidth;

    float rightArmAngle = 0.0f;
    float leftArmAngle = 0.0f;
    float rightLegAngle = 0.0f;
    float leftLegAngle = 0.0f;

    float rightArmPitch = 0.0f;
    float leftArmPitch = 0.0f;
    float rightLegPitch = 0.0f;
    float leftLegPitch = 0.0f;

    VECTOR leftArmOffset = VGet(-armOffset, m_size * 2, 0.0f);
    VECTOR rightArmOffset = VGet(armOffset, m_size * 2, 0.0f);

    VECTOR leftShoulderOffset = VGet(-armOffset, m_size * 3, 0.0f);
    VECTOR rightShoulderOffset = VGet(armOffset, m_size * 3, 0.0f);

    VECTOR leftLegOffset = VGet(-legOffset, m_size, 0.0f);
    VECTOR rightLegOffset = VGet(legOffset, m_size, 0.0f);

    VECTOR leftHipOffset = VGet(-legOffset, m_size * 2, 0.0f);
    VECTOR rightHipOffset = VGet(legOffset, m_size * 2, 0.0f);

    VECTOR leftEyeOffset = VGet(-m_size * 0.4f, m_size * 0.3f, m_size + 1.0f);
    VECTOR rightEyeOffset = VGet(m_size * 0.4f, m_size * 0.3f, m_size + 1.0f);

    VECTOR mouthOffset = VGet(0.0f, -m_size * 0.3f, m_size + 1.0f);
    
    if (m_isMoving) {
        m_animTime += 0.1f;
        ApplyWalkAnimation(leftArmOffset, rightArmOffset, leftLegOffset, rightLegOffset, leftArmPitch, rightArmPitch, leftLegPitch, rightLegPitch);
    }
    else {
        m_animTime = 0;
        ApplyIdolAnimation(leftArmOffset, rightArmOffset, leftLegOffset, rightLegOffset);
    }

    if (m_isAttacking) {
        ApplyNormalAttackAnimation(leftArmOffset, rightArmOffset, leftLegOffset, rightLegOffset, rightArmAngle, rightArmPitch);
    }

    VECTOR head = VGet(pos.x, pos.y + m_size * 4, pos.z);
    VECTOR leftArm = CalculatePartPositionFromJoint(leftArmOffset, leftShoulderOffset, leftArmAngle, leftArmPitch);
    VECTOR rightArm = CalculatePartPositionFromJoint(rightArmOffset,rightShoulderOffset,rightArmAngle,rightArmPitch);
    
    VECTOR body = VGet(pos.x, pos.y + m_size * 2, pos.z);
    VECTOR leftLeg = CalculatePartPositionFromJoint(leftLegOffset, leftHipOffset, leftLegAngle, leftLegPitch);
    VECTOR rightLeg = CalculatePartPositionFromJoint(rightLegOffset, rightHipOffset, rightLegAngle, rightLegPitch);

    VECTOR leftEye = VAdd(head, RotateXZ(leftEyeOffset));
    VECTOR rightEye = VAdd(head, RotateXZ(rightEyeOffset));
    VECTOR mouth = VAdd(head, RotateXZ(mouthOffset));

    DrawBox(head, m_size, m_size, m_size, frontColor);
    DrawBox(leftArm, legHalfWidth, m_size / 2, m_size, frontColor, leftArmAngle, leftArmPitch);
    DrawBox(rightArm, legHalfWidth, m_size / 2, m_size, frontColor, rightArmAngle, rightArmPitch);
    DrawBox(body, m_size, m_size, m_size, bodyColor);
    DrawBox(leftLeg, legHalfWidth, m_size / 2, m_size, bodyColor, leftLegAngle, leftLegPitch);
    DrawBox(rightLeg, legHalfWidth, m_size / 2, m_size, bodyColor, rightLegAngle, rightLegPitch);

    DrawBox(leftEye, m_size * 0.15f, 2.0f, m_size * 0.15f, BLACK);
    DrawBox(rightEye, m_size * 0.15f, 2.0f, m_size * 0.15f, BLACK);

    DrawBox(mouth, m_size * 0.2f, 1.0f, m_size * 0.3f, BLACK);

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

    float currentSpeed = m_speed;

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

void Player::Attack() {
    m_attackTriggered = true;

    m_attackArea.shape = AttackShape::Cone;
    m_attackArea.position = pos;
    m_attackArea.forward = GetForward();
    m_attackArea.range = 100.0f;
    m_attackArea.dotThreshold = 0.5f;
    m_attackArea.damage = 10;
}

void Player::NormalAttack() {
    m_attackTriggered = true;

    m_attackArea.shape = AttackShape::Cone;
    m_attackArea.position = pos;
    m_attackArea.forward = GetForward();
    m_attackArea.range = 100.0f;
    m_attackArea.dotThreshold = 0.5f;
    m_attackArea.damage = 10;

    m_isAttacking = true;
    m_attackTimer.Start(300);
}

void Player::ApplyIdolAnimation(VECTOR& leftArm, VECTOR& rightArm, VECTOR& leftLeg, VECTOR& rightLeg)
{

}

void Player::ApplyWalkAnimation(VECTOR& leftArm, VECTOR& rightArm, VECTOR& leftLeg, VECTOR& rightLeg, float& leftArmPitch, float& rightArmPitch, float& leftLegPitch, float& rightLegPitch)
{
    float swingAngle =
        sinf(m_animTime) *
        (30.0f * DX_PI_F / 180.0f);

    leftArmPitch = swingAngle;
    rightArmPitch = -swingAngle;

    leftLegPitch = -swingAngle;
    rightLegPitch = swingAngle;
}

void Player::ApplyNormalAttackAnimation(VECTOR& leftArm, VECTOR& rightArm, VECTOR& leftLeg, VECTOR& rightLeg, float& rightArmAngle, float& rightArmPitch) {
    float progress = m_attackTimer.GetProgress();

    float startAngle = 60.0f * DX_PI_F / 180.0f;
    float endAngle = -60.0f * DX_PI_F / 180.0f;

    rightArmAngle =
        startAngle + (endAngle - startAngle) * progress;

    // 攻撃中だけ腕を横向きにする
    rightArmPitch = -DX_PI_F / 2.0f;
}

void Player::DrawBox(VECTOR center, float halfWidth, float halfDepth, float halfHeight, int bodyColor, float addAngle, float pitchAngle) {
    VECTOR bottom[4];
    VECTOR top[4];

    CreateDrawVertices(center, bottom, top, halfWidth, halfDepth, halfHeight, addAngle, pitchAngle);

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

void Player::CreateDrawVertices(VECTOR center, VECTOR bottom[4], VECTOR top[4], float halfWidth, float halfDepth, float halfHeight, float addAngle, float pitchAngle) {
    VECTOR bottomLocal[4] = {
        VGet(-halfWidth, -halfHeight, -halfDepth),
        VGet(halfWidth, -halfHeight, -halfDepth),
        VGet(halfWidth, -halfHeight,  halfDepth),
        VGet(-halfWidth, -halfHeight,  halfDepth)
    };

    VECTOR topLocal[4] = {
        VGet(-halfWidth, halfHeight, -halfDepth),
        VGet(halfWidth, halfHeight, -halfDepth),
        VGet(halfWidth, halfHeight,  halfDepth),
        VGet(-halfWidth, halfHeight,  halfDepth)
    };

    float cosPitch = cosf(pitchAngle);
    float sinPitch = sinf(pitchAngle);

    float rotateAngle = angle.x + addAngle;
    float cosAngle = cosf(rotateAngle);
    float sinAngle = sinf(rotateAngle);

    for (int i = 0; i < 4; i++) {
        VECTOR pitchBottom = VGet(
            bottomLocal[i].x,
            bottomLocal[i].y * cosPitch - bottomLocal[i].z * sinPitch,
            bottomLocal[i].y * sinPitch + bottomLocal[i].z * cosPitch
        );

        VECTOR pitchTop = VGet(
            topLocal[i].x,
            topLocal[i].y * cosPitch - topLocal[i].z * sinPitch,
            topLocal[i].y * sinPitch + topLocal[i].z * cosPitch
        );
        
        VECTOR rotatedBottom = VGet(
            pitchBottom.x * cosAngle + pitchBottom.z * sinAngle,
            pitchBottom.y,
            -pitchBottom.x * sinAngle + pitchBottom.z * cosAngle
        );

        VECTOR rotatedTop = VGet(
            pitchTop.x * cosAngle + pitchTop.z * sinAngle,
            pitchTop.y,
            -pitchTop.x * sinAngle + pitchTop.z * cosAngle
        );

        bottom[i] = VAdd(center, rotatedBottom);
        top[i] = VAdd(center, rotatedTop);
    }
}

VECTOR Player::RotateXZ(VECTOR v)
{
    float cosAngle = cosf(angle.x);
    float sinAngle = sinf(angle.x);

    return VGet(v.x * cosAngle + v.z * sinAngle, v.y, -v.x * sinAngle + v.z * cosAngle);
}

VECTOR Player::CalculatePartPositionFromJoint(VECTOR partOffset, VECTOR jointOffset, float addAngle, float pitchAngle) {
    VECTOR jointToPart = VSub(partOffset, jointOffset);

    // 上下方向の回転
    float pitch = pitchAngle;

    float cosPitch = cosf(pitch);
    float sinPitch = sinf(pitch);

    VECTOR pitched = VGet(
        jointToPart.x,
        jointToPart.y * cosPitch - jointToPart.z * sinPitch,
        jointToPart.y * sinPitch + jointToPart.z * cosPitch
    );

    // プレイヤーの向き + アニメーションの角度
    float rotateAngle = angle.x + addAngle;

    float cosAngle = cosf(rotateAngle);
    float sinAngle = sinf(rotateAngle);

    VECTOR rotated = VGet(
        pitched.x * cosAngle + pitched.z * sinAngle,
        pitched.y,
        -pitched.x * sinAngle + pitched.z * cosAngle
    );

    // 関節のワールド座標
    VECTOR jointPosition =
        VAdd(pos, RotateXZ(jointOffset));

    return VAdd(jointPosition, rotated);
}

void Player::UpdateState() {
    float x = Input::GetAxisLX();
    float y = Input::GetAxisLY();
}

int Player::GetLayer() const {
    return m_layer;
}

VECTOR Player::GetForward() const {
    return VGet(sinf(angle.x), 0.0f, cosf(angle.x));
}

void Player::SetCameraAngle(Angle& a) { C_angle = a; }
#pragma endregion