#pragma once

class Player;
class Sector;

constexpr int SECTOR_SIZE = 100;
constexpr int SECTOR_X_COUNT = WORLD_WIDTH / SECTOR_SIZE;
constexpr int SECTOR_Y_COUNT = WORLD_HEIGHT / SECTOR_SIZE;

class SectorManager
{
public:
    SectorManager();
    ~SectorManager() = default;

    bool IsValidSector(int sectorX, int sectorY) const;
    int GetSectorX(short x) const;
    int GetSectorY(short y) const;

    void AddPlayer(std::shared_ptr<Player> player);
    void RemovePlayer(std::shared_ptr<Player> player);
    void UpdatePlayerSector(std::shared_ptr<Player> player);
    void BroadcastMove(std::shared_ptr<Player> player);
    void BroadcastAvatarInfoToNearbyPlayers(std::shared_ptr<Player> sourcePlayer);
    void SendNearbyPlayersToPlayer(std::shared_ptr<Player> targetPlayer);
    std::vector<int> GetNearbyPlayerIds(std::shared_ptr<Player> player);

private:
    std::vector<std::vector<Sector>> _sectors;
};

extern std::shared_ptr<SectorManager> GSectorManager;