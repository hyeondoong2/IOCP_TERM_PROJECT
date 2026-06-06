#pragma once

class Session; 

class Player
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

public:
    int _id = -1;
    std::string _name;

    short _x = 0;
    short _y = 0;

    int _visualId = 0;
    int _hp = 100;
    int _maxHp = 100;
    unsigned long long _exp = 0;
    unsigned char _level = 1;

    int _lastMoveTime = 0;

    int _sectorX = -1;
    int _sectorY = -1;

    PLAYER_STATE _state = PLAYER_STATE::NONE;

    std::weak_ptr<Session> _session;
};