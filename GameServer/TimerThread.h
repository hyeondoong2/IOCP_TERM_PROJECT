#pragma once

class TimerThread
{
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;
    using Duration = Clock::duration;

    TimerThread() = default;
    ~TimerThread() = default;

    void RegisterEvent(TIMER_EVENT timerEvent);
    void RunTimer();

    // 클래스 자체에 딱 하나 존재하는 함수라 static 선언..
    static TimePoint Now() noexcept
    {
        return Clock::now();
    }

private:
    // inline으로 선언해서 컴파일러가 최적화할 수 있도록 Functor로 선언
    struct EventComparer
    {
        bool operator()(const TIMER_EVENT& lhs, const TIMER_EVENT& rhs) const noexcept
        {
            if (lhs.wakeup_time != rhs.wakeup_time)
                return lhs.wakeup_time > rhs.wakeup_time;

            return lhs.sequence > rhs.sequence;
        }
    };

    void ProcessTimerEvent(const TIMER_EVENT& timerEvent);

private:
    std::priority_queue<TIMER_EVENT, std::vector<TIMER_EVENT>, EventComparer> _timer_queue;
    std::mutex _mutex;
    std::condition_variable _cv;

    uint64_t _sequence = 0;
};

