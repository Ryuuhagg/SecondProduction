#pragma once
#include"Character.h"
#include"Constant.h"
enum class EnemyState {
	Move,	// ˆÚ“®
};

class Enemy : public Character {
	EnemyState state = EnemyState::Move;
	bool m_isActive;
public:
	Enemy();
	void Init(int y, int x, int z, int id);
	void Update()override;
	void Draw()override;

	void Move();

	bool IsActive() const { return m_isActive; }
	VECTOR GetPosition() const { return pos; }
};