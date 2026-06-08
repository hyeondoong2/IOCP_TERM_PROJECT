#pragma once

class GameObject;
class Sector;
class Player;

constexpr int SECTOR_SIZE = 10;
constexpr int SECTOR_X_COUNT = WORLD_WIDTH / SECTOR_SIZE;
constexpr int SECTOR_Y_COUNT = WORLD_HEIGHT / SECTOR_SIZE;

constexpr int VIEW_RANGE = 5;

inline bool IsPlayer(int id) { return id < MAX_PLAYERS; }
inline bool IsNPC(int id) { return id >= NPC_ID_START; }

class SectorManager
{
public:
    SectorManager();
    ~SectorManager() = default;

    bool IsValidSector(int sectorX, int sectorY) const;
    int GetSectorX(short x) const;
    int GetSectorY(short y) const;

    void AddObject(std::shared_ptr<GameObject> object);
    void RemoveObject(std::shared_ptr<GameObject> object);
    void UpdateObjectSector(std::shared_ptr<GameObject> object);

    void SendNearbyObjectsToPlayer(std::shared_ptr<Player> player);

    std::unordered_set<int> GetNearbyObjectIds(std::shared_ptr<GameObject> object);

    bool CanSee(std::shared_ptr<GameObject> from, std::shared_ptr<GameObject> to) const;

private:
    std::vector<std::vector<Sector>> _sectors;
};

extern std::shared_ptr<SectorManager> GSectorManager;