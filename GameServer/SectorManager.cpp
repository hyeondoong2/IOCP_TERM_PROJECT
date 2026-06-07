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

void SectorManager::BroadcastMove(std::shared_ptr<GameObject> player)
{
    if (!player) return;

    S2C_MoveObject movePkt;
    movePkt.size = sizeof(S2C_MoveObject);
    movePkt.type = S2C_MOVE_OBJECT;
    movePkt.object_id = player->_id;
    movePkt.x = player->_x;
    movePkt.y = player->_y;

    auto myPlayer = std::dynamic_pointer_cast<Player>(player);
    if (myPlayer)
    {
        if (auto mySession = myPlayer->_session.lock())
        {
            mySession->DoSend(reinterpret_cast<const char*>(&movePkt));
        }
    }

    // 플레이어인 경우에만
    for (int nearbyId : GetNearbyObjectIds(player))
    {
        auto nearbyPlayer = GObjectManager->FindAs<Player>(nearbyId);
        if (nearbyPlayer)
        {
            if (auto session = nearbyPlayer->_session.lock())
            {
                session->DoSend(reinterpret_cast<const char*>(&movePkt));
            }
        }
    }
}

void SectorManager::BroadcastSpawnInfo(std::shared_ptr<GameObject> object)
{
    if (!object) return;

    bool hasNearbyPlayer = false;
    auto baseObject = std::static_pointer_cast<GameObject>(object);

    for (int nearbyId : GetNearbyObjectIds(object))
    {
        if (nearbyId != object->_id && nearbyId < MAX_PLAYERS)
        {
            auto nearbyPlayer = GObjectManager->FindAs<Player>(nearbyId);
            if (nearbyPlayer)
            {
                auto basePlayer = std::static_pointer_cast<GameObject>(nearbyPlayer);

                if (CanSee(baseObject, basePlayer))
                {
                    if (auto session = nearbyPlayer->_session.lock())
                    {
                        session->send_add_object_packet(baseObject);

                        hasNearbyPlayer = true;
                    }
                }
            }
        }
    }

    if (hasNearbyPlayer && object->_id >= MAX_PLAYERS)
    {
        auto npc = std::static_pointer_cast<NPC>(object);
        npc->WakeUp();
    }
}

void SectorManager::SendNearbyObjectsToPlayer(std::shared_ptr<Player> player)
{
    if (!player) return;

    std::unordered_set<int> current_view;
    auto basePlayer = std::static_pointer_cast<GameObject>(player);

    for (int nearbyId : GetNearbyObjectIds(basePlayer))
    {
        if (nearbyId == player->_id) continue;

        auto nearbyObj = GObjectManager->FindAs<GameObject>(nearbyId);
        if (nearbyObj && CanSee(basePlayer, nearbyObj))
        {
            current_view.insert(nearbyId);
        }
    }

    player->UpdateViewList(current_view);
}

std::vector<int> SectorManager::GetNearbyObjectIds(std::shared_ptr<GameObject> object)
{
    std::vector<int> result;
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
            for (int playerId : players)
            {
                if (playerId == object->_id) continue;
                result.push_back(playerId);
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
