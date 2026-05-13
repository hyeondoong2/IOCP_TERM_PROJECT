#include "pch.h"
#include "TimerThread.h"

void TimerThread::RegisterEvent(TIMER_EVENT timerEvent)
{
    bool WakeUp = false;

    {
        // lock_quard의 경우 생성자 외에 따로 lock 을 할 수 없음....
        std::lock_guard<std::mutex> lock(_mutex);
        timerEvent.sequence = _sequence++;

        if (_timer_queue.empty() || timerEvent.wakeup_time < _timer_queue.top().wakeup_time)
            WakeUp = true;

        _timer_queue.push(timerEvent);
    }

    // 스레드 깨우기...
    if (WakeUp)
        _cv.notify_one();
}

void TimerThread::RunTimer()
{
    // unique_lock의 경우 lock_guard와 달리 lock과 unlock을 자유롭게 할 수 있음
    // wait 함수가 인자로 unique lock을 받음
    std::unique_lock lock(_mutex);

    while (true)
    {
        // 큐에 알람이 들어올 때까지 대기
        // timer queue가 empty가 아닐 때까지 재우기
        _cv.wait(lock, [this]()
            {
                return _timer_queue.empty() == false;
            });

        // 알람이 들어온 경우 처리
        while (!_timer_queue.empty())
        {
            auto now = Now();
            auto next_wakeup_time = _timer_queue.top().wakeup_time;

            // 다음 알람이 아직 깨울 시간이 안된 경우, 깨울 시간까지 대기
            if (next_wakeup_time > now)
            {
                _cv.wait_until(lock, next_wakeup_time, [this, next_wakeup_time]
                    {
                        return _timer_queue.empty() || _timer_queue.top().wakeup_time < next_wakeup_time;
                    });
                continue;
            }

            TIMER_EVENT ready_event = _timer_queue.top();
            _timer_queue.pop();

            lock.unlock();

            // 타이머 이벤트 처리
            ProcessTimerEvent(ready_event);

            lock.lock();
        }
    }
}

void TimerThread::ProcessTimerEvent(const TIMER_EVENT& timerEvent)
{
    const int obj_id = timerEvent.obj_id;
    const int target_id = timerEvent.target_id;

    switch (timerEvent.event_type)
    {

    }
}
