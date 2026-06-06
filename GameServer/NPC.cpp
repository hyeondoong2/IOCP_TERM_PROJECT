#include "pch.h"
#include "NPC.h"

void NPC::Init(int id, short x, short y, const std::string& name)
{
    _id = id;
    _x = x;
    _y = y;
    _hp = 100; 

    _name = name;
}

void NPC::RandomMove()
{
    switch (rand() % 4)
    {
    case 0: _x++; break;
    case 1: _x--; break;
    case 2: _y++; break;
    case 3: _y--; break;
    }
}
