//Character.h
#pragma once
#include"DxLib.h"
#include"Constant.h"
#include"Timer.h"
#include<vector>
using namespace std;

// ダメージを受けるための共通クラス
class Damageable
{
public:
    virtual ~Damageable() = default;

    virtual void Damage(int damage) = 0;
    virtual bool IsAlive() const = 0;
};

class Character : public Damageable{
protected:
	float m_speed;
	VECTOR pos = VGet(0, 0, 0);

	int m_hp = 0;
	int m_maxHp = 0;
public:
	/// <summary>
	/// キャラクターの速度と最大HPを初期化するコンストラクタ。
	/// </summary>
	/// <param name="speed">キャラクターの移動速度を指定する浮動小数点数。</param>
	/// <param name="maxHp">キャラクターの最大HPを指定する整数。</param>
	Character(float speed, int maxHp):m_speed(speed), m_maxHp(maxHp), m_hp(maxHp){}
	virtual void Update() = 0;
	virtual void Draw() = 0;

    void Damage(int damage){
        m_hp -= damage;

        if (m_hp < 0)
            m_hp = 0;
    }

    bool IsAlive() const override {
        return m_hp > 0;
    }

    int GetHp() const {
        return m_hp;
    }

    int GetMaxHp() const {
        return m_maxHp;
    }

    VECTOR GetPosition() const {
        return pos;
    }
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

//共通関数
void DrawQuad3D(VECTOR a, VECTOR b, VECTOR c, VECTOR d, int color);



/**/
//6/8追加
//敵関連
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
