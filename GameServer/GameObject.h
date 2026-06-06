#pragma once

class GameObject
{
public:
    GameObject() = default;
    virtual ~GameObject() = default;

    int _id = -1;
    short _x = 0;
    short _y = 0;

    std::string _name;

    int _hp = 100;
    int _maxHp = 100;

    int _sectorX = -1;
    int _sectorY = -1;
};