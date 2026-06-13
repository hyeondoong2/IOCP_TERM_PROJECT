#include "pch.h"
#include "Player.h"
#include "GameObject.h"
#include "ObjectManager.h"
#include "NPC.h"
#include "Session.h"
#include "SectorManager.h"
#include "SessionManager.h"
#include "TimerThread.h"

void Player::InitFromLogin(
    int id,
    const std::string& name,
    short x,
    short y,
    uint8_t dbLevel,
    uint32_t dbExp,
    std::weak_ptr<Session> session)
{
    _id = id;
    _name = name;
    _x = x;
    _y = y;
    _level = dbLevel;
    _exp = dbExp;
    _originX = x;
    _originY = y;

    _state = OBJECT_STATE::IN_GAME;

    _session = session;
}

bool Player::IsInGame() const
{
    return _state == OBJECT_STATE::IN_GAME;
}

void Player::SendMovePacketToViewers()
{
    auto mySession = GetSession();

    S2C_MoveObject movePkt;
    movePkt.size = sizeof(S2C_MoveObject);
    movePkt.type = S2C_MOVE_OBJECT;
    movePkt.object_id = _id;
    movePkt.x = _x;
    movePkt.y = _y;
    movePkt.move_time = _clientMoveTime;

    if (mySession)
    {
        mySession->DoSend(reinterpret_cast<const char*>(&movePkt));
    }
    else
    {
        //std::cout << "npc send move packet" << std::endl;
    }

    for (auto& nearbyId : _viewList)
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

void Player::SendStatusChangePacket()
{
    auto mySession = GetSession();

    S2C_StatusChange statusPkt{};
    statusPkt.size = sizeof(S2C_StatusChange);
    statusPkt.type = S2C_STATUS_CHANGE;
    statusPkt.object_id = _id;
    statusPkt.hp = _hp;
    statusPkt.max_hp = _maxHp;
    statusPkt.level = _level;
    statusPkt.exp = _exp;

    if (mySession)
        mySession->DoSend(reinterpret_cast<const char*>(&statusPkt));
}

void Player::UpdateViewList(const std::unordered_set<int>& newViewList)
{
    auto mySession = GetSession();

    // Add
    for (int newId : newViewList)
    {
        if (_viewList.find(newId) == _viewList.end())
        {
            auto obj = GObjectManager->FindObject(newId);
            if (!obj) continue;

            if (mySession)
                mySession->send_add_object_packet(obj);

            if (IsPlayer(newId))
            {
                auto other = std::static_pointer_cast<Player>(obj);
                if (auto otherSession = other->_session.lock())
                    otherSession->send_add_object_packet(shared_from_this());
            }
            else if (mySession && IsNPC(newId))
            {
                //std::cout << "NPC " << obj->_id << "이(가) 플레이어 " << _id << "의 시야에 들어왔습니다.\n";
                std::static_pointer_cast<NPC>(obj)->WakeUp();
            }
        }
    }

    // Remove
    for (int oldId : _viewList)
    {
        if (newViewList.find(oldId) == newViewList.end())
        {
            if (mySession)
                mySession->send_remove_object_packet(oldId);

            if (IsPlayer(oldId))
            {
                auto other = GSessionManager->Find(oldId);
                if (other)
                {
                    other->send_remove_object_packet(_id);
                }
            }
        }
    }

    _viewList = newViewList;
}

void Player::Attack()
{
    auto mySession = GetSession();
    if (!mySession) return;

    S2C_AttackObject atkPkt{ sizeof(S2C_AttackObject), S2C_ATTACK_OBJECT, _id };

    if (mySession) mySession->DoSend(reinterpret_cast<const char*>(&atkPkt));
    for (int nearbyId : _viewList)
    {
        if (IsPlayer(nearbyId))
        {
            auto viewerSession = GSessionManager->Find(nearbyId);
            if (viewerSession) viewerSession->DoSend(reinterpret_cast<const char*>(&atkPkt));
        }
    }

    auto myPlayer = shared_from_this();
    std::unordered_set<int> hitNpcs;

    GSectorManager->ForEachNearbyNPC(myPlayer, [&](int npcId)
        {
            auto obj = GObjectManager->FindObject(npcId);
            if (!obj || obj->_state == OBJECT_STATE::DEAD) return;

            int dx = std::abs(myPlayer->_x - obj->_x);
            int dy = std::abs(myPlayer->_y - obj->_y);

            if (dx <= 1 && dy <= 1)
            {
                // 중복 타격 방지
                if (hitNpcs.find(npcId) == hitNpcs.end())
                {
                    auto npc = std::static_pointer_cast<NPC>(obj);
                    npc->OnDamaged(myPlayer->_id, 20);
                    hitNpcs.insert(npcId);
                }
            }
        });
}

void Player::OnDamaged(int attackerId, int damage)
{
    if (_state == OBJECT_STATE::DEAD) return;

    auto mySession = GetSession();
    if (!mySession) return;

    _hp -= damage;
    if (_hp < 0) _hp = 0;

    SendStatusChangePacket();

    if (_hp <= 0)
    {
        OnDeath(attackerId);
    }
    else
    {
        S2C_HitObject hitPkt{};
        hitPkt.size = sizeof(S2C_HitObject);
        hitPkt.type = S2C_HIT_OBJECT;
        hitPkt.object_id = _id;

        if (mySession)
        {
            mySession->DoSend(reinterpret_cast<const char*>(&hitPkt));
        }

        for (auto& nearbyId : _viewList)
        {
            if (!IsPlayer(nearbyId)) continue;
            auto nearbySession = GSessionManager->Find(nearbyId);
            if (nearbySession)
                nearbySession->DoSend(reinterpret_cast<const char*>(&hitPkt));
        }

        RegisterHeal();
    }
}

void Player::OnDeath(int attackerId)
{
    if (_state == OBJECT_STATE::DEAD) return;

    auto mySession = GetSession();
    if (!mySession) return;

    _hp = 0;
    _state = OBJECT_STATE::DEAD;

    S2C_DieObject diePkt{};
    diePkt.size = sizeof(S2C_DieObject);
    diePkt.type = S2C_DIE_OBJECT;
    diePkt.object_id = _id;

    if (mySession)
        mySession->DoSend(reinterpret_cast<const char*>(&diePkt));

    for (auto& nearbyId : _viewList)
    {
        if (!IsPlayer(nearbyId)) continue;
        auto session = GSessionManager->Find(nearbyId);
        if (session)
            session->DoSend(reinterpret_cast<const char*>(&diePkt));
    }

    TIMER_EVENT respawnEvent;
    respawnEvent.event_type = TIMER_EVENT_PLAYER_RESPAWN;
    respawnEvent.obj_id = _id;
    respawnEvent.wakeup_time = TimerThread::Now() + std::chrono::seconds(3);
    GTimerThread->RegisterEvent(respawnEvent);
}

void Player::Respawn()
{
    _hp = _maxHp;
    _exp = _exp / 2;
    _state = OBJECT_STATE::IN_GAME;

    _x = _originX;
    _y = _originY;

    _viewList.clear();

    auto selfPlayer = std::static_pointer_cast<Player>(shared_from_this());

    GSectorManager->UpdateObjectSector(selfPlayer);
    GSectorManager->SendNearbyObjectsToPlayer(selfPlayer);

    auto mySession = GetSession();
    if (!mySession) return;

    SendStatusChangePacket();

    if (mySession)
        mySession->send_add_object_packet(shared_from_this());
}

void Player::GetExp(int bonus)
{
    _exp += (_level * _level * 2 * bonus);
    if (_exp >= _level * 100) LevelUp(_exp);

    SendStatusChangePacket();
}

void Player::LevelUp(int currExp)
{
    _exp = currExp - (100 * _level);
    _level += 1;
}

void Player::RegisterHeal()
{
    if (_isHealed) return;
    _isHealed = true;

    TIMER_EVENT nextEvent;
    nextEvent.event_type = TIMER_EVENT_PLAYER_HEAL;
    nextEvent.obj_id = this->_id;
    nextEvent.wakeup_time = TimerThread::Now() + std::chrono::milliseconds(PLAYER_HEAL_INTERVAL);

    GTimerThread->RegisterEvent(nextEvent);
}

void Player::Heal()
{
    _isHealed = false;

    int heal_amount = _maxHp / 10;

    _hp += heal_amount;

    if (_hp > _maxHp)
    {
        _hp = _maxHp;
    }
    else
    {
        RegisterHeal();
    }
    
    SendStatusChangePacket();
}
