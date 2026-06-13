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
    _originX = x;
    _originY = y;
    _hp = 100;

    _name = name;
    _moveType = moveType;
    _battleType = battleType;
    _damage = damage;
    _state = OBJECT_STATE::IN_GAME;
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

    if (IsInAttackRange(targetId, 1)) return;

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
    nextEvent.wakeup_time = TimerThread::Now() + std::chrono::milliseconds(NPC_ATTACK_INTERVAL);

    GTimerThread->RegisterEvent(nextEvent);
}

void NPC::OnDamaged(int attackerId, int damage)
{
    if (_battleType == BATTLE_TYPE::PEACE) return;

    if (_state == OBJECT_STATE::DEAD) return;

    auto self = shared_from_this();
    _hp -= damage;

    S2C_HitObject hitPkt{};
    hitPkt.size = sizeof(S2C_HitObject);
    hitPkt.type = S2C_HIT_OBJECT;
    hitPkt.object_id = _id;

    // 근처에 있는 플레이어들에게 상태 전송
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
                    session->DoSend(reinterpret_cast<const char*>(&hitPkt));
            }
        });

    if (_hp <= 0)
    {
        OnDeath(attackerId);
    }
    else
    {
        if (!_attack_player)
        {
            RegisterAttack(attackerId);
        }
    }
}

void NPC::OnDeath(int attackerId)
{
    if (_state == OBJECT_STATE::DEAD) return;

    std::cout << "monster is dead" << std::endl;
    _state = OBJECT_STATE::DEAD;
    _active_npc = false;
    _attack_player = false;

    auto self = shared_from_this();
    S2C_DieObject diePkt{};
    diePkt.size = sizeof(S2C_DieObject);
    diePkt.type = S2C_DIE_OBJECT;
    diePkt.object_id = _id;

    GSectorManager->ForEachNearbyPlayer(self, [&](int playerId)
        {
            auto session = GSessionManager->Find(playerId);
            {
                if (session) session->DoSend(reinterpret_cast<const char*>(&diePkt));
                if (auto player = session->_owner.lock())
                {
                    player->_viewList.erase(_id);
                }
            }
        });

    auto attacker = GObjectManager->FindObject(attackerId);
    if (attacker && IsPlayer(attackerId))
    {
        auto player = std::static_pointer_cast<Player>(attacker);
        player->GetExp(GetKillExp());
    }

    TIMER_EVENT respawnEvent;
    respawnEvent.event_type = TIMER_EVENT_NPC_RESPAWN;
    respawnEvent.obj_id = _id;
    respawnEvent.wakeup_time = TimerThread::Now() + std::chrono::seconds(30);
    GTimerThread->RegisterEvent(respawnEvent);
}

void NPC::Respawn()
{
    _hp = _maxHp;      
    _x = _originX;        
    _y = _originY;

    GSectorManager->UpdateObjectSector(shared_from_this());

    _state = OBJECT_STATE::IN_GAME;
    _active_npc = true;    
    _attack_player = false;

    S2C_AddObject addPkt{};
    addPkt.size = sizeof(S2C_AddObject);
    addPkt.type = S2C_ADD_OBJECT;
    addPkt.object_id = _id;
    addPkt.x = _x;
    addPkt.y = _y;

    auto self = shared_from_this();
    GSectorManager->ForEachNearbyPlayer(self, [&](int playerId)
        {
            auto obj = GObjectManager->FindObject(playerId);
            if (!obj) return;

            if (!GSectorManager->CanSee(self, obj)) return;

            if (auto session = GSessionManager->Find(playerId))
            {
                session->DoSend(reinterpret_cast<const char*>(&addPkt));
            }
        });
}

bool NPC::IsInAttackRange(int targetId, int range)
{
    auto target = GObjectManager->FindObject(targetId);
    if (!target) return false;

    int distSq = (target->_x - _x) * (target->_x - _x)
        + (target->_y - _y) * (target->_y - _y);
    return distSq <= range * range;
}

void NPC::Attack(int targetId)
{
    if (_state == OBJECT_STATE::DEAD) return;

    auto target = GObjectManager->FindObject(targetId);
    if (!target)
    {
        _attack_player = false;
        return;
    }

    if (!IsInAttackRange(targetId))
    {
        _attack_player = false;
        return;
    }

    auto player = std::static_pointer_cast<Player>(target);
    player->OnDamaged(_id, _damage);

    auto self = shared_from_this();

    S2C_AttackObject attackPkt{};
    attackPkt.size = sizeof(S2C_AttackObject);
    attackPkt.type = S2C_ATTACK_OBJECT;
    attackPkt.object_id = _id;

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
                    session->DoSend(reinterpret_cast<const char*>(&attackPkt));
            }
        });

    _attack_player = false;
    RegisterAttack(targetId);
}

void NPC::WakeUp()
{
    if (_active_npc) return;
    _active_npc = true;

    TIMER_EVENT nextEvent;
    nextEvent.event_type = TIMER_EVENT_NPC_MOVE;
    nextEvent.obj_id = this->_id;
    nextEvent.wakeup_time = TimerThread::Now() + std::chrono::milliseconds(NPC_MOVE_INTERVAL);

    GTimerThread->RegisterEvent(nextEvent);
}

void NPC::UpdateMove()
{
    if (_state == OBJECT_STATE::DEAD) return;

    auto self = shared_from_this();
    bool isAggroing = false;

    // 1. 공격 타입에 따른 타겟 탐색 (PEACE는 타겟을 찾지 않음)
    int targetId = -1;
    if (_battleType == BATTLE_TYPE::AGRO)
    {
        targetId = FindNearbyPlayer(5); // 시야 범위 5
    }

    // 2. 타겟이 있을 때 (추적 or 공격)
    if (targetId != -1)
    {
        isAggroing = true;
        if (IsInAttackRange(targetId, 1))
        {
            // 공격 범위 안: 추적 멈추고 제자리에서 공격 이벤트 등록
            RegisterAttack(targetId);
        }
        else
        {
            // 공격 범위 밖: 추적
            DoAgroMove(targetId);
        }
    }
    // 3. 타겟이 없을 때 (Roaming or Fixed)
    else
    {
        // 평화로울 때 공격 타이머 취소 (기획: 멀어지면 안 때림)
        _attack_player = false;

        if (_moveType == MOVE_TYPE::ROAMING)
        {
            DoRoamingMove();
        }
        // FIXED는 아무것도 안 함 (가만히 있음)
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