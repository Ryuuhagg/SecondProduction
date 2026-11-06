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

float Timer::GetProgress() const
{
    if (!running || duration <= 0)
        return 0.0f;

    float progress = static_cast<float>(GetNowCount() - startTime) / static_cast<float>(duration);

    if (progress > 1.0f)
        progress = 1.0f;

    return progress;
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

Cooldown::Cooldown(int interval) : m_interval(interval){}

bool Cooldown::IsReady() const
{
    // まだ一度も使用していないならすぐ使える
    if (!m_started)
        return true;

    return GetNowCount() - m_lastTime >= m_interval;
}

void Cooldown::Reset()
{
    m_lastTime = GetNowCount();
    m_started = true;
}

void Cooldown::SetInterval(int interval)
{
    m_interval = interval;
}