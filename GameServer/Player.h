#pragma once
#include "GameObject.h"

class Session; 

class Player : public GameObject
{
public:
    Player() = default;
    ~Player() = default;

    void InitFromLogin(
        int id,
        const std::string& name,
        short x,
        short y,
        std::weak_ptr<Session> session);

    bool IsInGame() const;

    std::shared_ptr<Session> GetSession() override { return _session.lock(); }

public:
    int _visualId = 0;
    unsigned long long _exp = 0;
    unsigned char _level = 1;

    PLAYER_STATE _state = PLAYER_STATE::NONE; // state는 Player 전용으로 유지
    std::weak_ptr<Session> _session;
};