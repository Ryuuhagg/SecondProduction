//Character.h
#pragma once
#include"DxLib.h"
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

//ã§í ä÷êî
void DrawQuad3D(VECTOR a, VECTOR b, VECTOR c, VECTOR d, int color);

/**/
//6/8í«â¡
//ìGä÷òA
/*

*/
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
