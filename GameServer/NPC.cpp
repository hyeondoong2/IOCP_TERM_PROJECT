#include "pch.h"
#include "NPC.h"
#include "TimerThread.h"
#include "SectorManager.h"
#include "SessionManager.h"
#include "Session.h"
#include "ObjectManager.h"
#include "Player.h"

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

void NPC::UpdateMove()
{
    auto self = shared_from_this();
    RandomMove();
    GSectorManager->UpdateObjectSector(self);

    bool hasNearbyPlayer = BroadcastMoveToPlayers(self);

    if (hasNearbyPlayer)
    {
        TIMER_EVENT nextEvent;
        nextEvent.event_type = TIMER_EVENT_NPC_MOVE;
        nextEvent.obj_id = _id;
        nextEvent.wakeup_time = TimerThread::Now() + std::chrono::milliseconds(NPC_MOVE_INTERVAL);
        GTimerThread->RegisterEvent(nextEvent);
    }
    else
    {
        _active_npc = false;
    }
}

bool NPC::BroadcastMoveToPlayers(std::shared_ptr<GameObject> self)
{
    S2C_MoveObject movePkt;
    movePkt.size = sizeof(S2C_MoveObject);
    movePkt.type = S2C_MOVE_OBJECT;
    movePkt.object_id = _id;
    movePkt.x = _x;
    movePkt.y = _y;
    movePkt.move_time = _lastMoveTime;

    bool hasNearbyPlayer = false;

    // 섹터 탐색 1회로 add/move/remove 전부 처리
    for (auto& nearbyId : GSectorManager->GetNearbyObjectIds(self))
    {
        if (!IsPlayer(nearbyId)) continue;
        auto obj = GObjectManager->FindObject(nearbyId);
        if (!obj) continue;

        auto player = std::static_pointer_cast<Player>(obj);
        auto session = GSessionManager->Find(nearbyId);
        bool canSeeNow = GSectorManager->CanSee(self, obj);

        if (canSeeNow)
        {
            hasNearbyPlayer = true;
            if (!session) continue;

            if (player->IsInViewList(_id))
                session->DoSend(reinterpret_cast<const char*>(&movePkt));
            else
                session->send_add_object_packet(self);
        }
        else
        {
            if (player->IsInViewList(_id) && session)
                session->send_remove_object_packet(_id);
        }
    }

    return hasNearbyPlayer;
}