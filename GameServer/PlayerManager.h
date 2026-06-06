#pragma once

class Player;

class PlayerManager
{
public:
    void AddPlayer(std::shared_ptr<Player> player);
    void RemovePlayer(int playerId);
    std::shared_ptr<Player> FindPlayer(int playerId);

private:
    std::mutex _mutex;
    std::unordered_map<int, std::shared_ptr<Player>> _players;
};

extern std::shared_ptr<PlayerManager> GPlayerManager;