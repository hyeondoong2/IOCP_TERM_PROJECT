#pragma once
#include "GameObject.h"

struct Node
{
    short x, y;
    int g, h, f;
    Node* parent;

    Node(short _x, short _y, int _g, int _h, Node* _p = nullptr)
        : x(_x), y(_y), g(_g), h(_h), f(_g + _h), parent(_p)
    {
    }

    bool operator>(const Node& other) const
    {
        return f > other.f;
    }
};

const short dx[] = { 0, 0, -1, 1 };
const short dy[] = { -1, 1, 0, 0 };

class NPC : public GameObject
{
public:
    NPC() = default;
    ~NPC() = default;

    void Init(int id, short x, short y, const std::string& name,
        MOVE_TYPE moveType = MOVE_TYPE::ROAMING,
        BATTLE_TYPE battleType = BATTLE_TYPE::PEACE,
        int level = 1);

    void WakeUp();
    void UpdateMove();

    // 경험치
    int GetKillExp() const
    {
        int base = _level * _level * 2;
        return (_battleType == BATTLE_TYPE::AGRO) ? base * 2 : base;
    }

public:
    bool        _active_npc{ false };
    MOVE_TYPE   _moveType{ MOVE_TYPE::ROAMING };
    BATTLE_TYPE _battleType{ BATTLE_TYPE::PEACE };
    int         _level{ 1 };
    short       _originX{ 0 };  // 로밍 원점
    short       _originY{ 0 };

private:
    void RandomMove();
    void DoFixedMove();
    void DoRoamingMove();
    void DoAgroMove(int targetId);
    int  FindNearbyPlayer(int range);  // 범위 내 플레이어 탐색, 없으면 -1
    bool BroadcastMoveToPlayers(std::shared_ptr<GameObject> self);
};