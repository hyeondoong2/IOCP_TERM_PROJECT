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
    std::atomic<bool> _active_npc{ false }; // 반드시 atomic으로 선언

    void WakeUp();
};
