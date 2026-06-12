#pragma once

struct NPCSpawnInfo
{
    int         id;
    short       x, y;
    std::string name;
    MOVE_TYPE    moveType;
    BATTLE_TYPE  battleType;
    int         level;
};

class ScriptManager
{
public:
    void Init();
    void LoadNPCData(std::vector<NPCSpawnInfo>& out);
    bool IsTileBlocked(int x, int y);

private:
    lua_State* _L = nullptr;
    std::vector<std::vector<int>> _mapTiles;

    MOVE_TYPE   ParseMoveType(const std::string& s);
    BATTLE_TYPE ParseBattleType(const std::string& s);
};

extern std::shared_ptr<ScriptManager> GScriptManager;