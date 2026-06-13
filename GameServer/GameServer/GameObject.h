#pragma once

class Session;

class GameObject : public std::enable_shared_from_this<GameObject>
{
public:
    GameObject() = default;
    virtual ~GameObject() = default;

    virtual void OnDamaged(int attackerId, int damage) {}
    virtual void OnDeath(int attackerId) {}
    virtual void Respawn() {}
public:
    int _id = -1;
    short _x = 0;
    short _y = 0;
    short _originX = 0;
    short _originY = 0;

    std::string _name;

    int _hp = 100;
    int _maxHp = 100;

    int _sectorX = -1;
    int _sectorY = -1;


    OBJECT_STATE _state = OBJECT_STATE::NONE;
};