#include "pch.h"
#include "SectorManager.h"
#include "Player.h"
#include "Sector.h"
#include "PlayerManager.h"
#include "Session.h"

std::shared_ptr<SectorManager> GSectorManager = std::make_shared<SectorManager>();

SectorManager::SectorManager()
{
    _sectors.resize(SECTOR_Y_COUNT);
    for (int y = 0; y < SECTOR_Y_COUNT; ++y)
    {
        _sectors[y].resize(SECTOR_X_COUNT);
    }
}

bool SectorManager::IsValidSector(int sectorX, int sectorY) const
{
    return sectorX >= 0 && sectorX < SECTOR_X_COUNT &&
        sectorY >= 0 && sectorY < SECTOR_Y_COUNT;
}

int SectorManager::GetSectorX(short x) const { return x / SECTOR_SIZE; }
int SectorManager::GetSectorY(short y) const { return y / SECTOR_SIZE; }

void SectorManager::AddPlayer(std::shared_ptr<Player> player)
{
    if (!player) return;

    int sectorX = GetSectorX(player->_x);
    int sectorY = GetSectorY(player->_y);

    if (!IsValidSector(sectorX, sectorY)) return;

    player->_sectorX = sectorX;
    player->_sectorY = sectorY;

    _sectors[sectorY][sectorX].AddPlayer(player->_id);
}

void SectorManager::RemovePlayer(std::shared_ptr<Player> player)
{
    if (!player) return;

    int sectorX = player->_sectorX;
    int sectorY = player->_sectorY;

    if (!IsValidSector(sectorX, sectorY)) return;

    _sectors[sectorY][sectorX].RemovePlayer(player->_id);

    player->_sectorX = -1;
    player->_sectorY = -1;
}

void SectorManager::UpdatePlayerSector(std::shared_ptr<Player> player)
{
    if (!player) return;

    int oldSectorX = player->_sectorX;
    int oldSectorY = player->_sectorY;

    int newSectorX = GetSectorX(player->_x);
    int newSectorY = GetSectorY(player->_y);

    if (!IsValidSector(newSectorX, newSectorY)) return;
    if (oldSectorX == newSectorX && oldSectorY == newSectorY) return;

    if (IsValidSector(oldSectorX, oldSectorY))
    {
        _sectors[oldSectorY][oldSectorX].RemovePlayer(player->_id);
    }

    _sectors[newSectorY][newSectorX].AddPlayer(player->_id);

    player->_sectorX = newSectorX;
    player->_sectorY = newSectorY;
}

void SectorManager::BroadcastMove(std::shared_ptr<Player> player)
{
    if (!player) return;

    S2C_MoveObject movePkt;
    movePkt.size = sizeof(S2C_MoveObject);
    movePkt.type = S2C_MOVE_OBJECT;
    movePkt.object_id = player->_id;
    movePkt.x = player->_x;
    movePkt.y = player->_y;
    movePkt.move_time = player->_lastMoveTime;

    // 나 자신
    if (auto mySession = player->_session.lock())
    {
        mySession->DoSend(reinterpret_cast<const char*>(&movePkt));
    }

    // 다른 플레이어
    for (int nearbyId : GetNearbyPlayerIds(player))
    {
        if (nearbyId == player->_id) continue;

        auto nearbyPlayer = GPlayerManager->FindPlayer(nearbyId);
        if (nearbyPlayer)
        {
            if (auto session = nearbyPlayer->_session.lock())
            {
                session->DoSend(reinterpret_cast<const char*>(&movePkt));
            }
        }
    }
}

void SectorManager::BroadcastAvatarInfoToNearbyPlayers(std::shared_ptr<Player> sourcePlayer)
{
    if (!sourcePlayer) return;

    for (int nearbyId : GetNearbyPlayerIds(sourcePlayer))
    {
        auto nearbyPlayer = GPlayerManager->FindPlayer(nearbyId);
        if (nearbyPlayer)
        {
            if (auto session = nearbyPlayer->_session.lock())
            {
                session->send_avatar_packet(sourcePlayer);
            }
        }
    }
}

void SectorManager::SendNearbyPlayersToPlayer(std::shared_ptr<Player> targetPlayer)
{
    auto mySession = targetPlayer ? targetPlayer->_session.lock() : nullptr;
    if (!mySession) return;

    for (int nearbyId : GetNearbyPlayerIds(targetPlayer))
    {
        auto nearbyPlayer = GPlayerManager->FindPlayer(nearbyId);
        if (auto session = nearbyPlayer->_session.lock())
        {
            session->send_avatar_packet(nearbyPlayer);
        }
    }
}

std::vector<int> SectorManager::GetNearbyPlayerIds(std::shared_ptr<Player> player)
{
    std::vector<int> result;
    if (!player) return result;

    int centerX = player->_sectorX;
    int centerY = player->_sectorY;

    if (!IsValidSector(centerX, centerY)) return result;

    for (int y = centerY - 1; y <= centerY + 1; ++y)
    {
        for (int x = centerX - 1; x <= centerX + 1; ++x)
        {
            if (!IsValidSector(x, y)) continue;

            const auto& players = _sectors[y][x].GetPlayers();
            for (int playerId : players)
            {
                if (playerId == player->_id) continue;
                result.push_back(playerId);
            }
        }
    }
    return result;
}