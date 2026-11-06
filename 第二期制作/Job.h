#pragma once
enum class JobType {
	Warrior,
	Mage,
};

/// <summary>
/// ジョブの基本性能を決める構造体
/// </summary>
struct JobData {
	int maxHp;
	float speed;

	int attackPower;
};

extern JobData Warrior;
extern JobData Mage;