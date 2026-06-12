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

    bool IsInViewList(int id) const
    {
        return _viewList.count(id) > 0;
    }

    std::shared_ptr<Session> GetSession() { return _session.lock(); }

    void SendMovePacketToViewers();
    void UpdateViewList(const std::unordered_set<int>& newViewList);

    void OnDamaged(int attackerId, int damage) override;
    void OnDeath(int attackerId) override;
    void Respawn() override;
    void GetExp(int exp);

public:
    int _visualId = 0;
    unsigned long long _exp = 0;
    unsigned char _level = 1;

    std::weak_ptr<Session> _session;

    std::unordered_set<int> _viewList;
};