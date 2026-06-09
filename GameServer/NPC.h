#pragma once
#include "GameObject.h"

class NPC : public GameObject
{
public:
    NPC() = default;
    ~NPC() = default;

    void Init(int id, short x, short y, const std::string& name);

    void RandomMove();
public:
    bool _active_npc{ false };

    void WakeUp();
    void UpdateMove();

private:
    std::unordered_set<int> GetVisiblePlayers();
    void BroadcastMoveToPlayers(const std::unordered_set<int>& oldView,
        const std::unordered_set<int>& newView);
};
