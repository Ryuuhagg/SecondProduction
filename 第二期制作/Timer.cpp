#include "Timer.h"

void Timer::Start(int ms)
{
    startTime = GetNowCount();
    duration = ms;
    running = true;
}

bool Timer::IsFinished() const
{
    if (!running) return false;

    return GetNowCount() - startTime >= duration;
}

bool Timer::IsRunning() const
{
    return running;
}

void Timer::Stop()
{
    running = false;
}

void Stopwatch::Reset()
{
    startTime = GetNowCount();
    pauseStartTime = 0;
    totalPauseTime = 0;
    paused = false;
}

bool Stopwatch::After(int ms) const
{
    return GetNowCount() - startTime >= ms;
}

int Stopwatch::Elapsed()const
{
    if (paused) {
        return pauseStartTime - startTime - totalPauseTime;
    }

    return GetNowCount() - startTime - totalPauseTime;
}

void Stopwatch::Pause() {
    if (!paused) {
        pauseStartTime = GetNowCount();
        paused = true;
    }
}

void Stopwatch::Resume() {
    if (paused) {
        totalPauseTime += GetNowCount() - pauseStartTime;
        paused = false;
    }
}
