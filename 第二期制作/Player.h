#pragma once
#include"Character.h"
#include"AttackArea.h"
class Player : public Character {
	bool m_isMoving = false;
	bool m_isDashing = false;
	bool m_isAttacking = false;

	Timer m_attackTimer;

	float m_animTime = 0;
	float m_size = 10.0f;

	Damageable* m_target = nullptr;
	AttackArea m_attackArea;
	VECTOR m_targetPos;

	bool m_attackTriggered = false;
	///ヒロシが追加　レイヤー
	int m_layer = 0;
	Cooldown m_coolDown;

	CharaState m_state = CharaState::Idle;
	Angle angle = { 0.0f, 0.3f };
	Angle C_angle = { 0.0f, 0.3f };
public:
	Player();
	~Player();

	void Init();
	void Update()override;
	void Draw() override;

	void Move();
	void Attack();
	void NormalAttack();

	void ApplyIdolAnimation(VECTOR& leftArm, VECTOR& rightArm, VECTOR& leftLeg, VECTOR& rightLeg);
	void ApplyWalkAnimation(VECTOR& leftArm, VECTOR& rightArm, VECTOR& leftLeg, VECTOR& rightLeg, float& leftArmPitch, float& rightArmPitch, float& leftLegPitch, float& rightLegPitch);
	void ApplyNormalAttackAnimation(VECTOR& leftArm, VECTOR& rightArm, VECTOR& leftLeg, VECTOR& rightLeg, float& rightArmAngle, float& rightArmPitch);

	void DrawBox(VECTOR center, float halfWidth, float halfDepth, float halfHeight, int bodyColor, float addAngle = 0.0f, float pitchAngle = 0.0f);
	void CreateDrawVertices(VECTOR center, VECTOR bottom[4], VECTOR top[4], float halfWidth, float halfDepth, float halfHeight, float addAngle = 0.0f, float pitchAngle = 0.0f);
	VECTOR RotateXZ(VECTOR v);

	VECTOR CalculatePartPositionFromJoint(VECTOR partOffset,VECTOR jointOffset,float addAngle,float pitchAngle);
	// 2026-06-02: 罠や投げ弾からPlayerへダメージを渡し、HUDで残りライフを見せるため追加。
	int GetLayer()const;
	void UpdateState();

	Angle getAngle() { return angle; }
	VECTOR GetForward() const;
	void SetCameraAngle(Angle& a);
	bool IsAttackTriggered() const {
		return m_attackTriggered;
	}
	const AttackArea& GetAttackArea() const {
		return m_attackArea;
	}
};