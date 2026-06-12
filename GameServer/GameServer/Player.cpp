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
    std::weak_ptr<Session> session)
{
    _id = id;
    _name = name;
    _x = x;
    _y = y;
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
    movePkt.move_time = _lastMoveTime;
    //std::cout << _lastMoveTime << std::endl;

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

void Player::OnDamaged(int attackerId, int damage)
{
    if (_state == OBJECT_STATE::DEAD) return;

    _hp -= damage;
    if (_hp < 0) _hp = 0;

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
    {
        mySession->DoSend(reinterpret_cast<const char*>(&statusPkt));
    }

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
    }
}

void Player::OnDeath(int attackerId)
{
    if (_state == OBJECT_STATE::DEAD) return;

    auto mySession = GetSession();

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

    if (mySession)
        mySession->send_add_object_packet(shared_from_this());
}

void Player::GetExp(int exp)
{
}
