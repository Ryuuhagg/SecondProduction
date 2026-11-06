#include"Player.h"
#include"MapLoader.h"
#include"Input.h"
#include"GameObjects.h"
#include"Constant.h"
using namespace PlayerTuning;
#pragma region === Player ===
Player::Player() :Character(10, 10000), m_normalAttackCoolDown(1000), m_skill1CoolDown(1000),m_skill2CoolDown(1000)
{
    Init();
}
Player::~Player() {
}

void Player::Init() {

    pos = GetStartPosition();

    angle = { 0.0f, 1.5f };
    SetJob(JobType::Warrior);
}

void Player::Update() {
    m_attackTriggered = false;
    m_isMoving = false;
    
    Move();
    UpdateState();
    SetGameLoadedLayer(m_layer);

    if (Input::IsActionTrigger(Action::Attack)) {
        NormalAttack();
    }



    if (m_isNomalAttacking && m_attackTimer.IsFinished()) {
        m_isNomalAttacking = false;
        m_attackTimer.Stop();
    }

}

void Player::Draw() {
    const int bodyColor = GetColor(255, 0, 0);
    const int frontColor = GetColor(255, 255, 0);

    float legHalfWidth = default_size * 0.35f;
    float armHalfWidth = default_size * 0.35f;
    float legOffset = default_size * 0.55f;
    float armOffset = default_size + armHalfWidth;

    PlayerPose pose;

    VECTOR leftArmOffset = VGet(-armOffset, default_size * 2, 0.0f);
    VECTOR rightArmOffset = VGet(armOffset, default_size * 2, 0.0f);

    VECTOR leftShoulderOffset = VGet(-armOffset, default_size * 3, 0.0f);
    VECTOR rightShoulderOffset = VGet(armOffset, default_size * 3, 0.0f);

    VECTOR leftLegOffset = VGet(-legOffset, default_size, 0.0f);
    VECTOR rightLegOffset = VGet(legOffset, default_size, 0.0f);

    VECTOR leftHipOffset = VGet(-legOffset, default_size * 2, 0.0f);
    VECTOR rightHipOffset = VGet(legOffset, default_size * 2, 0.0f);

    VECTOR leftEyeOffset = VGet(-default_size * 0.4f, default_size * 0.3f, default_size + 1.0f);
    VECTOR rightEyeOffset = VGet(default_size * 0.4f, default_size * 0.3f, default_size + 1.0f);

    VECTOR mouthOffset = VGet(0.0f, -default_size * 0.3f, default_size + 1.0f);
    
    if (m_isMoving) {
        m_animTime += 0.1f;
        ApplyWalkAnimation(pose);
    }
    else {
        m_animTime = 0;
        ApplyIdolAnimation(pose);
    }

    if (m_isNomalAttacking) {
        ApplyAttackAnimation(pose);
    }

    VECTOR head = VGet(pos.x, pos.y + default_size * 4, pos.z);
    VECTOR leftArm = CalculatePartPositionFromJoint(leftArmOffset, leftShoulderOffset, pose.leftArm.angle, pose.leftArm.pitch);
    VECTOR rightArm = CalculatePartPositionFromJoint(rightArmOffset,rightShoulderOffset, pose.rightArm.angle, pose.rightArm.pitch);
    
    VECTOR body = VGet(pos.x, pos.y + default_size * 2, pos.z);
    VECTOR leftLeg = CalculatePartPositionFromJoint(leftLegOffset, leftHipOffset, pose.leftLeg.angle, pose.leftLeg.pitch);
    VECTOR rightLeg = CalculatePartPositionFromJoint(rightLegOffset, rightHipOffset, pose.rightLeg.angle, pose.rightLeg.pitch);

    VECTOR leftEye = VAdd(head, RotateXZ(leftEyeOffset));
    VECTOR rightEye = VAdd(head, RotateXZ(rightEyeOffset));
    VECTOR mouth = VAdd(head, RotateXZ(mouthOffset));

    DrawBox(head, default_size, default_size, default_size, frontColor);
    DrawBox(leftArm, armHalfWidth, default_size / 2, default_size, frontColor, pose.leftArm.angle, pose.leftArm.pitch);
    DrawBox(rightArm, armHalfWidth, default_size / 2, default_size, frontColor, pose.rightArm.angle, pose.rightArm.pitch);
    DrawBox(body, default_size, default_size, default_size, bodyColor);
    DrawBox(leftLeg, legHalfWidth, default_size / 2, default_size, bodyColor, pose.leftLeg.angle, pose.leftLeg.pitch);
    DrawBox(rightLeg, legHalfWidth, default_size / 2, default_size, bodyColor, pose.rightLeg.angle, pose.rightLeg.pitch);

    DrawBox(leftEye, default_size * 0.15f, 2.0f, default_size * 0.15f, BLACK);
    DrawBox(rightEye, default_size * 0.15f, 2.0f, default_size * 0.15f, BLACK);

    DrawBox(mouth, default_size * 0.2f, 1.0f, default_size * 0.3f, BLACK);

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

void Player::NormalAttack() {
    if (!m_normalAttackCoolDown.IsReady()) {
        return;
    }
    m_attackTriggered = true;

    m_attackData = WarriorSlamAttackArea;

    m_attackData.position = pos;
    m_attackData.forward = GetForward();
    m_attackData.power += m_jobData.attackPower;
    m_normalAttackCoolDown.SetInterval(m_attackData.coolDown);
    m_normalAttackCoolDown.Reset();

    m_isNomalAttacking = true;
    switch (m_attackData.animationType) {
    case AttackAnimationType::Swing:
        m_attackTimer.Start(SwingAttackTime);
        break;
    case AttackAnimationType::Slam:
        m_attackTimer.Start(SlamAttackTime);
        break;
    }
}

void Player::Skill1() {
    if (!m_skill1CoolDown.IsReady()) {
        return;
    }
    m_attackTriggered = true;

    m_attackData = WarriorSlamAttackArea;

    m_attackData.position = pos;
    m_attackData.forward = GetForward();
    m_attackData.power += m_jobData.attackPower;
    m_normalAttackCoolDown.SetInterval(m_attackData.coolDown);
    m_normalAttackCoolDown.Reset();

    m_isNomalAttacking = true;
    //m_attackTimer.Start(m_attackData.animationTime);
}

void Player::Skill2() {

}

void Player::ApplyIdolAnimation(PlayerPose& pose)
{

}

void Player::ApplyWalkAnimation(PlayerPose& pose)
{
    float swingAngle =
        sinf(m_animTime) *
        (30.0f * DX_PI_F / 180.0f);

    pose.leftArm.pitch = swingAngle;
    pose.rightArm.pitch = -swingAngle;

    pose.leftLeg.pitch = -swingAngle;
    pose.rightLeg.pitch = swingAngle;
}

void Player::ApplyAttackAnimation(PlayerPose& pose){
    switch (m_attackData.animationType) {
    case AttackAnimationType::Swing:
        ApplySwingAnimation(pose);
        break;
    case AttackAnimationType::Slam:
        ApplySlamAnimation(pose);
        break;
    }
}

void Player::ApplySwingAnimation(PlayerPose& pose) {
    float progress = m_attackTimer.GetProgress();

    float startAngle = 60.0f * DX_PI_F / 180.0f;
    float endAngle = -60.0f * DX_PI_F / 180.0f;

    pose.rightArm.angle =
        startAngle + (endAngle - startAngle) * progress;

    // 攻撃中だけ腕を横向きにする
    pose.rightArm.pitch = -DX_PI_F / 2.0f;
}

void Player::ApplySlamAnimation(PlayerPose& pose) {
    const float upPitch = -DX_PI_F;
    const float downPitch = -DX_PI_F / 4.0f;
    const float progress = m_attackTriggered ? 0.0f : m_attackTimer.GetProgress();

    pose.rightArm.pitch = upPitch + (downPitch - upPitch) * progress;
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

void Player::SetJob(JobType job) {
    switch (job) {
    case JobType::Warrior:
        m_jobData = Warrior;
        break;
    case JobType::Mage:
        m_jobData = Mage;
        break;
    }

    m_speed = m_jobData.speed;
}
#pragma endregion