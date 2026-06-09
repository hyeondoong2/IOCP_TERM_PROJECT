#include "pch.h"
#include "NPC.h"
#include "TimerThread.h"
#include "SectorManager.h"
#include "SessionManager.h"
#include "Session.h"

void NPC::Init(int id, short x, short y, const std::string& name)
{
    _id = id;
    _x = x;
    _y = y;
    _hp = 100; 

    _name = name;
}

void NPC::RandomMove()
{
    short nextX = _x;
    short nextY = _y;

    switch (rand() % 4)
    {
    case 0: nextX++; break;
    case 1: nextX--; break;
    case 2: nextY++; break;
    case 3: nextY--; break;
    }

    if (nextX >= 0 && nextX < WORLD_WIDTH) _x = nextX;
    if (nextY >= 0 && nextY < WORLD_HEIGHT) _y = nextY;
}

void NPC::SendMovePacketToViewers()
{
    S2C_MoveObject movePkt;
    movePkt.size = sizeof(S2C_MoveObject);
    movePkt.type = S2C_MOVE_OBJECT;
    movePkt.object_id = _id;
    movePkt.x = _x;
    movePkt.y = _y;
    movePkt.move_time = _lastMoveTime;
    //std::cout << _lastMoveTime << std::endl;


    for (auto& nearbyId : GSectorManager->GetNearbyObjectIds(shared_from_this()))
    {
        if (IsPlayer(nearbyId))
        {
            auto nearbyPlayer = GSessionManager->Find(nearbyId);
            if (nearbyPlayer)
            {
                nearbyPlayer->DoSend(reinterpret_cast<const char*>(&movePkt));
            }
        }
    }
}

void NPC::WakeUp()
{
    if (_active_npc) return;
    _active_npc = true;

    TIMER_EVENT nextEvent;
    nextEvent.event_type = TIMER_EVENT_NPC_MOVE;
    nextEvent.obj_id = this->_id;
    nextEvent.wakeup_time = TimerThread::Now() + std::chrono::milliseconds(1000);

    GTimerThread->RegisterEvent(nextEvent);
}
