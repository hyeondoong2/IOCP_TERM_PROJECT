#include "pch.h"
#include "Player.h"

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