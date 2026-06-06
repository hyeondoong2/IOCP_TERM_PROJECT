#pragma once

class Sector
{
public:
    void AddPlayer(int playerId)
    {
        _players.insert(playerId);
    }

    void RemovePlayer(int playerId)
    {
        _players.erase(playerId);
    }

    const std::unordered_set<int>& GetPlayers() const
    {
        return _players;
    }

private:
    std::unordered_set<int> _players;
};