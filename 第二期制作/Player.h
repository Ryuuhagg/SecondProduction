#pragma once
#include"Character.h"
#include"AttackData.h"
#include"Job.h"
#include"Weapon.h"
#include"Constant.h"
#include <deque>
using namespace PlayerTuning;
using namespace WeaponData;
struct PartPose {
	float angle = 0.0f;
	float pitch = 0.0f;
};

struct PlayerPose {
	PartPose leftArm;
	PartPose rightArm;
	PartPose leftLeg;
	PartPose rightLeg;
};

struct SwordTrailPoint {
	VECTOR grip;
	VECTOR tip;
};

class Player : public Character {
	// 状態を管理する変数
	bool m_isMoving = false;
	bool m_isDashing = false;
	bool m_isNomalAttacking = false;
	bool m_isSkill1 = false;
	bool m_isSkill2 = false;
	// 攻撃用のタイマー
	Timer m_attackTimer;
	// アニメーションの時間
	float m_animTime = 0;
	// 各攻撃のクールタイム
	Cooldown m_normalAttackCoolDown;
	Cooldown m_skill1CoolDown;
	Cooldown m_skill2CoolDown;

	// 攻撃対象
	Damageable* m_target = nullptr;
	AttackData m_attackData;
	VECTOR m_targetPos;

	bool m_attackTriggered = false;
	// キャラクターの職業
	JobType m_jobType = JobType::Warrior;
	JobData m_jobData;
	///ヒロシが追加　レイヤー
	int m_layer = 0;
	
	// 武器
	Sword m_sword{ swordLength, swordHalfWidth };
	// 武器の軌跡を描く用のやつ
	deque<SwordTrailPoint> m_swordTrail;
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
	void NormalAttack();
	void Skill1();
	void Skill2();

	void DrawWeapon(VECTOR rightShoulderOffset, const PlayerPose& pose);
	// 待機中のアニメーション（今は空）
	void ApplyIdolAnimation(PlayerPose& pose);
	// 歩きのアニメーション
	void ApplyWalkAnimation(PlayerPose& pose);
	// 攻撃のアニメーション
	void ApplyAttackAnimation(PlayerPose& pose, AttackData& attack);
	// 腕を横に振る
	void ApplySwingAnimation(PlayerPose& pose);
	// 腕を縦に振る
	void ApplySlamAnimation(PlayerPose& pose);
	// 四角を作る関数
	void DrawBox(VECTOR center, float halfWidth, float halfDepth, float halfHeight, int bodyColor, float addAngle = 0.0f, float pitchAngle = 0.0f);
	// 四角の角の部分を作る関数
	void CreateDrawVertices(VECTOR center, VECTOR bottom[4], VECTOR top[4], float halfWidth, float halfDepth, float halfHeight, float addAngle = 0.0f, float pitchAngle = 0.0f);
	// 角度のやつ
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
	const AttackData& GetAttackArea() const {
		return m_attackData;
	}

	void SetJob(JobType job);
};