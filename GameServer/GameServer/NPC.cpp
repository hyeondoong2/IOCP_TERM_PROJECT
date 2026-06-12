#include "pch.h"
#include "NPC.h"
#include "TimerThread.h"
#include "SectorManager.h"
#include "SessionManager.h"
#include "Session.h"
#include "ObjectManager.h"
#include "Player.h"
#include "Collision.h"

void NPC::Init(int id, short x, short y, const std::string& name, int damage,
    MOVE_TYPE moveType, BATTLE_TYPE battleType)
{
    _id = id;
    _x = x;
    _y = y;
    _hp = 100;

    _name = name;
    _moveType = moveType;
    _battleType = battleType;
    _damage = damage;
}

void NPC::DoFixedMove()
{
}

void NPC::DoRoamingMove()
{
    int dir = rand() % 4;
    short nextX = _x + dx[dir];
    short nextY = _y + dy[dir];

    if (!isCollision(nextX, nextY))
    {
        _x = nextX;
        _y = nextY;
    }
}

void NPC::DoAgroMove(int targetId)
{
    auto targetObj = GObjectManager->FindObject(targetId);
    if (!targetObj) return;

    short destX = targetObj->_x;
    short destY = targetObj->_y;

    int distSq = (destX - _x) * (destX - _x) + (destY - _y) * (destY - _y);
    if (distSq <= 1) return;

    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> openList;

    std::unordered_set<int> closedList;

    openList.emplace(_x, _y, 0, std::abs(destX - _x) + std::abs(destY - _y), nullptr);

    int maxSearchDepth = 50;
    int searchCount = 0;

    Node* bestNode = nullptr;
    std::vector<Node*> allNodes;

    while (!openList.empty() && searchCount < maxSearchDepth)
    {
        Node current = openList.top();
        openList.pop();

        int currentKey = current.y * WORLD_WIDTH + current.x;

        if (closedList.count(currentKey)) continue;
        closedList.insert(currentKey);

        Node* currentNodePtr = new Node(current);
        allNodes.push_back(currentNodePtr);

        if ((current.x == destX && current.y == destY) ||
            (std::abs(current.x - destX) + std::abs(current.y - destY) == 1))
        {
            bestNode = currentNodePtr;
            break;
        }

        for (int i = 0; i < 4; ++i)
        {
            short nx = current.x + dx[i];
            short ny = current.y + dy[i];

            if (isCollision(nx, ny)) continue;

            int nextKey = ny * WORLD_WIDTH + nx;
            if (closedList.count(nextKey)) continue;

            int g = current.g + 1;
            int h = std::abs(destX - nx) + std::abs(destY - ny);
            openList.emplace(nx, ny, g, h, currentNodePtr);
        }
        searchCount++;
    }

    if (bestNode != nullptr && bestNode->parent != nullptr)
    {
        while (bestNode->parent->parent != nullptr)
        {
            bestNode = bestNode->parent;
        }

        _x = bestNode->x;
        _y = bestNode->y;
    }

    for (auto node : allNodes) delete node;
}

int NPC::FindNearbyPlayer(int range)
{
    int targetId = -1;
    int minDistance = range * range;

    auto self = shared_from_this();

    GSectorManager->ForEachNearbyPlayer(self, [&](int playerId)
        {

            auto obj = GObjectManager->FindObject(playerId);
            if (!obj) return;

            int distSq = (obj->_x - _x) * (obj->_x - _x) + (obj->_y - _y) * (obj->_y - _y);

            if (distSq <= minDistance)
            {
                minDistance = distSq;
                targetId = playerId;
            }
        });

    return targetId;
}

void NPC::RegisterAttack(int targetId)
{
    if (_attack_player) return;
    _attack_player = true;

    TIMER_EVENT nextEvent;
    nextEvent.event_type = TIMER_EVENT_NPC_ATTACK;
    nextEvent.obj_id = this->_id;
    nextEvent.target_id = targetId;
    nextEvent.wakeup_time = TimerThread::Now() + std::chrono::milliseconds(1000);

    GTimerThread->RegisterEvent(nextEvent);
}

void NPC::Ondamaged(int attackerId, int damage)
{
    auto self = shared_from_this();
    _hp -= damage;

    S2C_StatusChange statusPkt;
    statusPkt.size = sizeof(S2C_StatusChange);
    statusPkt.type = S2C_STATUS_CHANGE;
    statusPkt.object_id = _id;
    statusPkt.hp = (_hp < 0) ? 0 : _hp; 

    GSectorManager->ForEachNearbyPlayer(self, [&](int playerId)
        {
            auto obj = GObjectManager->FindObject(playerId);
            if (!obj) return;

            auto player = std::static_pointer_cast<Player>(obj);
            auto session = GSessionManager->Find(playerId);
            if (!session) return;

            bool canSeeNow = GSectorManager->CanSee(self, obj);

            if (canSeeNow)
            {
                if (player->IsInViewList(_id))
                    session->DoSend(reinterpret_cast<const char*>(&statusPkt));
            }
        });

    if (_hp <= 0)
    {
        // 몬스터 사망 처리
    }
    else
    {
        if (!_attack_player)
        {
            RegisterAttack(attackerId);
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

void NPC::UpdateMove()
{
    auto self = shared_from_this();

    bool hasTarget = false;
    if (_battleType == BATTLE_TYPE::AGRO)
    {
        int targetId = FindNearbyPlayer(5);
        if (targetId != -1)
        {
            DoAgroMove(targetId);
            hasTarget = true;
            RegisterAttack(targetId);
        }
    }


    if (!hasTarget)
    {
        if (_moveType == MOVE_TYPE::ROAMING)
        {
            DoRoamingMove();
        }
        else if (_moveType == MOVE_TYPE::FIXED)
        {
            DoFixedMove();
        }
    }

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

    GSectorManager->ForEachNearbyPlayer(self, [&](int playerId)
        {
            auto obj = GObjectManager->FindObject(playerId);
            if (!obj) return;

            auto player = std::static_pointer_cast<Player>(obj);
            auto session = GSessionManager->Find(playerId);
            if (!session) return;

            hasNearbyPlayer = true;
            bool canSeeNow = GSectorManager->CanSee(self, obj);

            if (canSeeNow)
            {
                if (player->IsInViewList(_id))
                    session->DoSend(reinterpret_cast<const char*>(&movePkt));
                else
                    session->send_add_object_packet(self);
            }
            else
            {
                if (player->IsInViewList(_id))
                    session->send_remove_object_packet(_id);
            }
        });

    return hasNearbyPlayer;
}