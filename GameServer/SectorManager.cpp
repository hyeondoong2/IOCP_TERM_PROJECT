#include "pch.h"
#include "SectorManager.h"
#include "GameObject.h"
#include "Sector.h"
#include "ObjectManager.h"
#include "Session.h"
#include "Player.h"
#include "TimerThread.h"
#include "NPC.h"

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

void SectorManager::AddObject(std::shared_ptr<GameObject> player)
{
    if (!player) return;

    int sectorX = GetSectorX(player->_x);
    int sectorY = GetSectorY(player->_y);

    if (!IsValidSector(sectorX, sectorY)) return;

    player->_sectorX = sectorX;
    player->_sectorY = sectorY;

    _sectors[sectorY][sectorX].AddObject(player->_id);
}

void SectorManager::RemoveObject(std::shared_ptr<GameObject> player)
{
    if (!player) return;

    int sectorX = player->_sectorX;
    int sectorY = player->_sectorY;

    if (!IsValidSector(sectorX, sectorY)) return;

    _sectors[sectorY][sectorX].RemoveObject(player->_id);

    player->_sectorX = -1;
    player->_sectorY = -1;
}

void SectorManager::UpdateObjectSector(std::shared_ptr<GameObject> player)
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
        _sectors[oldSectorY][oldSectorX].RemoveObject(player->_id);
    }

    _sectors[newSectorY][newSectorX].AddObject(player->_id);

    player->_sectorX = newSectorX;
    player->_sectorY = newSectorY;
}


void SectorManager::SendNearbyObjectsToPlayer(std::shared_ptr<Player> player)
{
    if (!player) return;

    // 플레이어 근처에 있는 Object 중 보이는 Object를 플레이어 시야 리스트에 추가

    std::unordered_set<int> current_view;
    auto basePlayer = std::static_pointer_cast<GameObject>(player);

    for (auto& nearbyId : GetNearbyObjectIds(basePlayer))
    {
        if (nearbyId == player->_id) continue;

        auto nearbyObj = GObjectManager->FindObject(nearbyId);
        if (nearbyObj && CanSee(basePlayer, nearbyObj))
        {
            current_view.insert(nearbyId);
        }
    }

    player->UpdateViewList(current_view);
}

std::unordered_set<int> SectorManager::GetNearbyObjectIds(std::shared_ptr<GameObject> object)
{
    std::unordered_set<int> result;
    if (!object) return result;

    int centerX = object->_sectorX;
    int centerY = object->_sectorY;

    if (!IsValidSector(centerX, centerY)) return result;

    for (int y = centerY - 1; y <= centerY + 1; ++y)
    {
        for (int x = centerX - 1; x <= centerX + 1; ++x)
        {
            if (!IsValidSector(x, y)) continue;

            const auto& players = _sectors[y][x].GetObjects();
            for (auto& playerId : players)
            {
                if (playerId == object->_id) continue;
                result.insert(playerId);
            }
        }
    }
    return result;
}

bool SectorManager::CanSee(std::shared_ptr<GameObject> from, std::shared_ptr<GameObject> to) const
{
    if (!from || !to) return false;

    int dx = std::abs(from->_x - to->_x);
    int dy = std::abs(from->_y - to->_y);

    return (dx <= VIEW_RANGE) && (dy <= VIEW_RANGE);
}
