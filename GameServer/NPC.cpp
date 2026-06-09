#include "pch.h"
#include "NPC.h"
#include "TimerThread.h"
#include "SectorManager.h"
#include "SessionManager.h"
#include "Session.h"
#include "ObjectManager.h"

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
    auto oldView = GetVisiblePlayers();

    RandomMove();
    GSectorManager->UpdateObjectSector(shared_from_this());

    auto newView = GetVisiblePlayers();

    BroadcastMoveToPlayers(oldView, newView);

    if (!newView.empty())
    {
        TIMER_EVENT nextEvent;
        nextEvent.event_type = TIMER_EVENT_NPC_MOVE;
        nextEvent.obj_id = _id;
        nextEvent.wakeup_time = TimerThread::Now() + std::chrono::milliseconds(1000);
        GTimerThread->RegisterEvent(nextEvent);
    }
    else
    {
        _active_npc = false;
    }
}

std::unordered_set<int> NPC::GetVisiblePlayers()
{
    std::unordered_set<int> result;
    auto self = shared_from_this();
    for (auto& nearbyId : GSectorManager->GetNearbyObjectIds(self))
    {
        if (IsPlayer(nearbyId) &&
            GSectorManager->CanSee(self, GObjectManager->FindObject(nearbyId)))
            result.insert(nearbyId);
    }
    return result;
}

void NPC::BroadcastMoveToPlayers(const std::unordered_set<int>& oldView, const std::unordered_set<int>& newView)
{
    S2C_MoveObject movePkt;
    movePkt.size = sizeof(S2C_MoveObject);
    movePkt.type = S2C_MOVE_OBJECT;
    movePkt.object_id = _id;
    movePkt.x = _x;
    movePkt.y = _y;
    movePkt.move_time = _lastMoveTime;

    for (int id : newView)
    {
        auto session = GSessionManager->Find(id);
        if (!session) continue;
        if (oldView.count(id) == 0)
            session->send_add_object_packet(shared_from_this());
        else
            session->DoSend(reinterpret_cast<const char*>(&movePkt));
    }

    for (int id : oldView)
    {
        if (newView.count(id) == 0)
        {
            auto session = GSessionManager->Find(id);
            if (session)
                session->send_remove_object_packet(_id);
        }
    }
}
