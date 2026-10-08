#pragma once
#include"Constant.h"
#include"Player.h"
#include"Character.h"

class Camera {
	Player& p;
	Angle cameraAngle = { 0.0f, 0.3f };
	float distance;
	int frontLight;
public:
	Camera(Player& p);
	void Init();
	void Update();
	void Draw();

	void MoveAngle();
};