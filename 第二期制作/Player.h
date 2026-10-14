#pragma once
#include"Character.h"

class Player : public Character {
	bool m_isMoving = false;
	bool m_isDashing = false;

	float m_animTime = 0;
	float m_size = 10.0f;

	Damageable* m_target = nullptr;
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

	void ApplyIdolAnimation(VECTOR& leftArm, VECTOR& rightArm, VECTOR& leftLeg, VECTOR& rightLeg);
	void ApplyWalkAnimation(VECTOR& leftArm, VECTOR& rightArm, VECTOR& leftLeg, VECTOR& rightLeg);

	void DrawBox(VECTOR center, float halfWidth, float halfDepth, float halfHeight, int bodyColor);
	void CreateDrawVertices(VECTOR center, VECTOR bottom[4], VECTOR top[4], float halfWidth, float halfDepth, float halfHeight);
	VECTOR RotateXZ(VECTOR v);
	// 2026-06-02: 罠や投げ弾からPlayerへダメージを渡し、HUDで残りライフを見せるため追加。
	int GetLayer()const;
	void UpdateState();

	Angle getAngle() { return angle; }
	VECTOR GetForward() const;
	void SetCameraAngle(Angle& a);
	bool IsAttackTriggered() const {
		return m_attackTriggered;
	}
};