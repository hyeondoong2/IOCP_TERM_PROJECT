#include "pch.h"
#include "PlayerManager.h"
#include "Player.h" 

std::shared_ptr<PlayerManager> GPlayerManager = std::make_shared<PlayerManager>();

void PlayerManager::AddPlayer(std::shared_ptr<Player> player)
{
    if (!player)
        return;

    std::lock_guard<std::mutex> lock(_mutex);
    _players[player->_id] = player;
}

void PlayerManager::RemovePlayer(int playerId)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _players.erase(playerId);
}

std::shared_ptr<Player> PlayerManager::FindPlayer(int playerId)
{
    std::lock_guard<std::mutex> lock(_mutex);

    auto it = _players.find(playerId);

    if (it == _players.end())
        return nullptr;

    return it->second;
}