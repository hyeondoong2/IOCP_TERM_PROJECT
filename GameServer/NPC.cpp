#include "pch.h"
#include "NPC.h"
#include "TimerThread.h"

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
    bool expected = false;
    if (!_active_npc.compare_exchange_strong(expected, true))
        return;

    TIMER_EVENT nextEvent;
    nextEvent.event_type = TIMER_EVENT_NPC_MOVE;
    nextEvent.obj_id = this->_id;
    nextEvent.wakeup_time = TimerThread::Now() + std::chrono::milliseconds(100);

    GTimerThread->RegisterEvent(nextEvent);
}
