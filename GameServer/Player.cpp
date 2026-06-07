#include "pch.h"
#include "Player.h"
#include "GameObject.h"
#include "ObjectManager.h"
#include "NPC.h"
#include "Session.h"

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

    _state = PLAYER_STATE::IN_GAME;

    _session = session;
}

bool Player::IsInGame() const
{
    return _state == PLAYER_STATE::IN_GAME;
}

void Player::UpdateViewList(const std::unordered_set<int>& newViewList)
{
    auto session = _session.lock();
    if (!session) return;

    for (int newId : newViewList)
    {
        if (_viewList.find(newId) == _viewList.end())
        {
            auto obj = GObjectManager->FindAs<GameObject>(newId);
            if (obj)
            {
                session->send_add_object_packet(obj);

                if (newId >= MAX_PLAYERS)
                {
                    auto npc = std::static_pointer_cast<NPC>(obj);
                    npc->WakeUp();
                }
            }
        }
    }

    for (int oldId : _viewList)
    {
        if (newViewList.find(oldId) == newViewList.end())
        {
            session->send_remove_object_packet(oldId);
        }
    }

    _viewList = newViewList;
}
