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
        int damage,
        MOVE_TYPE moveType = MOVE_TYPE::ROAMING,
        BATTLE_TYPE battleType = BATTLE_TYPE::PEACE);

    void WakeUp();
    void UpdateMove();
    void RegisterAttack(int targetId);
    void Attack(int targetId);
    bool IsInAttackRange(int targetId, int range = 2);

    void OnDamaged(int attackerId, int damage) override;
    void OnDeath(int attackerId) override;
    void Respawn() override;

    int GetKillExp() const
    {
        int base =  1;

        if (_battleType == BATTLE_TYPE::AGRO) base *= 2;
        if (_moveType == MOVE_TYPE::ROAMING) base *= 2;
        
        return base;
    }

public:
    bool        _active_npc{ false };
    bool        _attack_player{ false };
    MOVE_TYPE   _moveType{ MOVE_TYPE::ROAMING };
    BATTLE_TYPE _battleType{ BATTLE_TYPE::PEACE };
    int         _level{ 1 };
    int         _damage{ 0 };

private:
    void DoFixedMove();
    void DoRoamingMove();
    void DoAgroMove(int targetId);
    int  FindNearbyPlayer(int range); 
    bool BroadcastMoveToPlayers(std::shared_ptr<GameObject> self);
};