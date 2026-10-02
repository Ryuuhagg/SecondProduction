//Character.h
#pragma once
#include"DxLib.h"
#include"Model.h"
#include"Constant.h"
#include<vector>
using namespace std;
class Character {
protected:
	float speed;
	VECTOR pos = VGet(0, 0, 0);
public:
	Character(float speed):speed(speed){}
	virtual void Update() = 0;
	virtual void Draw() = 0;
	VECTOR getVECTOR(){ return pos; };
};

struct Angle {
	float x;
	float y;
};

enum class CharaState {
	Jump,
	Idle,
	Walk,
	Dash
};


class Player : public Character {
	int m_model = 0;
	int m_animeIndex = 0;
	int m_Size = 0;
	int m_currentAnimNo = -1;
	
	int m_animIdle = 1;
	int m_animWalk = 2;
	int m_animJump = 0;

	bool m_isMoving = false;
	bool m_isDashing = false;

	float y = 0;
	float vy = 0;
	float m_gravity = -0.6f;
	bool m_isGround = true;

	int m_life = 0;

	///ヒロシが追加　レイヤー
	int m_layer = 0;

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

	void Jump();
	// 2026-06-02: 罠や投げ弾からPlayerへダメージを渡し、HUDで残りライフを見せるため追加。
	void Damage();
	bool IsAlive() const;
	int GetLife() const;
	int GetLayer()const;
	void UpdateState();
	void UpdateAnimation();

	void ChangeModel(const ModelData& data);

	Angle getAngle() { return angle; }

	void SetCameraAngle(Angle& a);
};

class Camela : public Character {
	Player& p;
	Angle camelaAngle = {0.0f, 0.3f };
	float distance;
	int frontLight;
public:
	Camela(Player& p);
	void Init();
	void Update()override;
	void Draw() override;

	void MoveAngle();
};
/**/
//6/8追加
//敵関連
struct Pos2 {
	int x;
	int z;
};

vector<Pos2> FindPathBFS(int layer, int startX, int startZ, int goalX, int goalZ);

vector<MapNode> FindMapPathBFS(
	int startLayer,
	int startX,
	int startZ,
	int goalLayer,
	int goalX,
	int goalZ);

enum class EnemyState {
	Patrol, // 巡回
	Chase   // 追跡
};

class Enemy : public Character {
	EnemyState state = EnemyState::Patrol; 
	vector<Pos2> patrolPoints;
	int patrolIndex = 0;

	float chaseRange = 900.0f;

	vector<Pos2> path;
	int pathTimer = 0;

	int m_model = 0;
	int m_animeIndex = 0;
	int m_Size = 0;
	int m_currentAnimNo = -1;

	int m_animIdle = 1;
	int m_animWalk = 2;
	int m_animJump = 0;

	CharaState m_state = CharaState::Idle;

    int m_layer = 0;
    float y = 0;

	int m_id;
    bool isActive;

	VECTOR m_playerPos;
	int m_playerLayer;
	// 7/17 敵が0.5の壁を越えられるように
	void MoveWithHalfStep(VECTOR nextPos);
public:
    Enemy();
    void Init(int y, int x, int z, int id);
    void Update()override;
    void Draw()override;
    void DrawLayer(int drawLayer);

	void ChangeModel(const ModelData& data);
    void Move();

	void MoveToPlayer();
	void MovePatrol();

	void MoveToCell(Pos2 next);

	bool IsHitPlayer(VECTOR playerPos, int playerLayer) const;

	void SetPlayerPos(VECTOR playerPos);
	void SetPlayerLayer(int playerLayer);
	int GetLayer()const{ return m_layer; }
    bool IsActive() const { return isActive; }
    VECTOR GetPosition() const { return pos; }
};

class EnemyManager {
    vector<Enemy> enemies;
    vector<int> pool;
public:
	EnemyManager();
    void Init();
    void Update(VECTOR playerPos, int playerLayer);
    void Draw();
    void DrawLayer(int drawLayer);
    int GetEnemyCount() const;
    bool GetEnemyPosition(int index, VECTOR& pos, int& layer) const;

	bool IsHitPlayer(VECTOR playerPos, int playerLayer) const;
};

extern EnemyManager e;
/*
class EnemyBullet {
public:
    void Init();
    void Update();
    void Draw();
};

class EnemyBulletManager {
    vector<EnemyBullet> bullets;
    vector<int> pool;
public:
    void Init();
    void Update();
    void Draw();
};
*/

//アイテム系
enum class ItemType {
	KEY,

};
class Item {
	VECTOR pos;
public:
	void Init(VECTOR pos, ItemType type);
	void Update();
	void Draw();
};

class ItemManager {
	vector<Item> items;
public:
	void Init();
	void Update();
	void Draw();
};