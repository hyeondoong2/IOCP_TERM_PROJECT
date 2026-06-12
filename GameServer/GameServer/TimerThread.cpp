#include "pch.h"
#include "TimerThread.h"]
#include "ObjectManager.h"
#include "SectorManager.h"
#include "SessionManager.h"
#include "NPC.h"
#include "GameLogicThread.h"
#include "Player.h"
#include "GameObject.h"
#include "Session.h"

std::shared_ptr<TimerThread> GTimerThread = std::make_shared<TimerThread>();

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
    std::unique_lock lock(_mutex);
    std::vector<TIMER_EVENT> readyEvents;
    readyEvents.reserve(32);

    while (_running)
    {
        _cv.wait(lock, [this]()
            {
                return !_timer_queue.empty() || !_running;
            });

        if (!_running) break;

        while (!_timer_queue.empty())
        {
            auto now = Now();
            auto next_wakeup_time = _timer_queue.top().wakeup_time;

            if (next_wakeup_time > now)
            {
                _cv.wait_until(lock, next_wakeup_time, [this, next_wakeup_time]
                    {
                        return !_running || _timer_queue.empty() ||
                            _timer_queue.top().wakeup_time < next_wakeup_time;
                    });
                continue;
            }

            // 만료된 이벤트 한번에 수집 (lock 잡은 상태)
            readyEvents.clear();
            while (!_timer_queue.empty() &&
                _timer_queue.top().wakeup_time <= now)
            {
                readyEvents.push_back(_timer_queue.top());
                _timer_queue.pop();
            }

            // lock 해제 후 일괄 처리 
            lock.unlock();
            for (const TIMER_EVENT& event : readyEvents)
                ProcessTimerEvent(event);
            lock.lock();
        }
    }
    std::cout << "TimerThread 종료\n";
}
void TimerThread::Stop()
{
    _running = false;
    _cv.notify_all();
}

void TimerThread::ProcessTimerEvent(const TIMER_EVENT& timerEvent)
{
    const int obj_id = timerEvent.obj_id;
    const int target_id = timerEvent.target_id;

    switch (timerEvent.event_type)
    {
    case TIMER_EVENT_MOVE:
    case TIMER_EVENT_NPC_MOVE:
    {
        //std::cout << "move npc timer event processed" << std::endl;
        GGameLogicThread->PostEvent([obj_id]()
            {
                auto npc = std::static_pointer_cast<NPC>(GObjectManager->FindObject(obj_id));
                if (!npc) return;
                npc->UpdateMove();
            });
        break;
    }
    case TIMER_EVENT_NPC_ATTACK:
    {
        GGameLogicThread->PostEvent([obj_id, target_id]()
            {
                auto npc = std::static_pointer_cast<NPC>(GObjectManager->FindObject(obj_id));
                if (!npc) return;
                npc->Attack(target_id);

            });
        break;
    }
    case TIMER_EVENT_NPC_RESPAWN:
    {
        GGameLogicThread->PostEvent([obj_id = timerEvent.obj_id]()
            {
                auto npc = std::static_pointer_cast<NPC>(GObjectManager->FindObject(obj_id));
                if (!npc) return;
                npc->Respawn();
            });
        break;
    }

    case TIMER_EVENT_PLAYER_RESPAWN:
    {
        GGameLogicThread->PostEvent([obj_id = timerEvent.obj_id]()
            {
                auto player = std::static_pointer_cast<Player>(GObjectManager->FindObject(obj_id));
                if (!player) return;
                player->Respawn();
            });
        break;
    }
    default:
        break;
    }
}




