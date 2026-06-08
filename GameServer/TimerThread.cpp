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
    // unique_lock의 경우 lock_guard와 달리 lock과 unlock을 자유롭게 할 수 있음
    // wait 함수가 인자로 unique lock을 받음
    std::unique_lock lock(_mutex);

    while (_running)
    {
        // 큐에 알람이 들어올 때까지 대기
        // timer queue가 empty가 아닐 때까지 재우기
        _cv.wait(lock, [this]()
            {
                return !_timer_queue.empty() || !_running;
            });

        if (!_running) break;

        std::cout << "[Timer] 루프 시작! 큐 크기: " << _timer_queue.size() << std::endl;

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
                        return !_running || _timer_queue.empty() || _timer_queue.top().wakeup_time < next_wakeup_time;
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
        GGameLogicThread->PostEvent([obj_id = timerEvent.obj_id]()
            {
                auto npc = std::static_pointer_cast<NPC>(GObjectManager->FindObject(obj_id));
                if (!npc) return;

                npc->RandomMove();
                GSectorManager->UpdateObjectSector(npc);

                // npc 시야 리스트 업데이트
                // npc는 플레이어만 시야 리스트에 넣어주면 댐
                std::unordered_set<int> current_view;

                for (int nearbyId : GSectorManager->GetNearbyObjectIds(npc))
                {
                    if (nearbyId == npc->_id) continue;

                    auto nearbyPlayer = std::static_pointer_cast<Player>(GObjectManager->FindObject(nearbyId));
                    if (nearbyPlayer && GSectorManager->CanSee(npc, nearbyPlayer))
                    {
                        current_view.insert(nearbyId);
                    }
                }
                npc->UpdateViewList(current_view);

                npc->SendMovePacketToViewers();

                if (!current_view.empty())
                {
                    TIMER_EVENT nextEvent;
                    nextEvent.event_type = TIMER_EVENT_NPC_MOVE;
                    nextEvent.obj_id = obj_id;
                    nextEvent.wakeup_time = TimerThread::Now() + std::chrono::milliseconds(1000);

                    GTimerThread->RegisterEvent(nextEvent);
                }
                else
                {
                    npc->_active_npc = false;
                }

            });
        break;
    }
    default:
        break;
    }
}




