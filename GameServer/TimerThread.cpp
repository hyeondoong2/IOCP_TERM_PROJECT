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
        GGameLogicThread->PostEvent([obj_id = timerEvent.obj_id]()
            {
                auto npc = std::static_pointer_cast<NPC>(GObjectManager->FindObject(obj_id));
                if (!npc) return;

                std::unordered_set<int> oldViewPlayers;
                for (auto& nearbyId : GSectorManager->GetNearbyObjectIds(npc))
                {
                    if (IsPlayer(nearbyId) && GSectorManager->CanSee(npc, GObjectManager->FindObject(nearbyId)))
                        oldViewPlayers.insert(nearbyId);
                }

                npc->RandomMove();
                GSectorManager->UpdateObjectSector(npc);

                // 이동 후 보이는 플레이어 목록
                std::unordered_set<int> newViewPlayers;
                for (auto& nearbyId : GSectorManager->GetNearbyObjectIds(npc))
                {
                    if (IsPlayer(nearbyId) && GSectorManager->CanSee(npc, GObjectManager->FindObject(nearbyId)))
                        newViewPlayers.insert(nearbyId);
                }

                // 새로 보이는 플레이어 → add + move 패킷
                S2C_MoveObject movePkt;
                movePkt.size = sizeof(S2C_MoveObject);
                movePkt.type = S2C_MOVE_OBJECT;
                movePkt.object_id = obj_id;
                movePkt.x = npc->_x;
                movePkt.y = npc->_y;
                movePkt.move_time = npc->_lastMoveTime;

                for (int id : newViewPlayers)
                {
                    auto session = GSessionManager->Find(id);
                    if (!session) continue;

                    if (oldViewPlayers.count(id) == 0)
                        session->send_add_object_packet(npc); // 새로 시야에 들어옴
                    else
                        session->DoSend(reinterpret_cast<const char*>(&movePkt)); // 계속 보임
                }

                // 시야에서 사라진 플레이어 → remove 패킷
                for (int id : oldViewPlayers)
                {
                    if (newViewPlayers.count(id) == 0)
                    {
                        auto session = GSessionManager->Find(id);
                        if (session)
                            session->send_remove_object_packet(obj_id);
                    }
                }

                if (!newViewPlayers.empty())
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




