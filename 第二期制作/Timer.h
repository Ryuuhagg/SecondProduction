#pragma once
#include"Dxlib.h"
class Timer {
	int startTime = 0;
	int duration = 0;
	bool running;
public:
	// 引数のミリ秒分runningがtrueになる
	void Start(int ms);
	// Startで入れた数を超えたらtrueになる
	bool IsFinished() const;
	// runningの状態を取れる
	bool IsRunning() const;
	// runningをfalseにする
	void Stop();
};

class Stopwatch {
private:
	int startTime = 0;
	int pauseStartTime = 0;
	int totalPauseTime = 0;
	bool paused = false;
public:
	//今の時間をstartTimerにセット
	void Reset(); 
	//引数の時間を超えたらtrue
	bool After(int ms) const;
	//Resetしてから何ミリ秒たっているか
	int Elapsed() const;

	void Pause();
	void Resume();

	void Stop();
};