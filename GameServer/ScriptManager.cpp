#include "pch.h"
#include "ScriptManager.h"

std::shared_ptr<ScriptManager> GScriptManager = std::make_shared<ScriptManager>();

void ScriptManager::Init()
{
    _L = luaL_newstate();
    luaL_openlibs(_L);

    if (luaL_dofile(_L, "scripts/map_data.lua") != LUA_OK)
    {
        std::cerr << "map_data.lua 로드 실패: " << lua_tostring(_L, -1) << std::endl;
        return;
    }
    if (luaL_dofile(_L, "scripts/npc_data.lua") != LUA_OK)
    {
        std::cerr << "npc_data.lua 로드 실패: " << lua_tostring(_L, -1) << std::endl;
        return;
    }

    // 맵 타일 캐싱
    lua_getglobal(_L, "map");
    int rows = luaL_len(_L, -1);
    for (int r = 1; r <= rows; ++r)
    {
        lua_rawgeti(_L, -1, r);
        int cols = luaL_len(_L, -1);
        std::vector<int> row;
        for (int c = 1; c <= cols; ++c)
        {
            lua_rawgeti(_L, -1, c);
            row.push_back(lua_tointeger(_L, -1));
            lua_pop(_L, 1);
        }
        _mapTiles.push_back(row);
        lua_pop(_L, 1);
    }
    lua_pop(_L, 1);
}

bool ScriptManager::IsTileBlocked(int x, int y)
{
    if (y < 0 || y >= (int)_mapTiles.size()) return true;
    if (x < 0 || x >= (int)_mapTiles[y].size()) return true;
    return _mapTiles[y][x] == 1;
}

void ScriptManager::LoadNPCData(std::vector<NPCSpawnInfo>& out)
{
    lua_getglobal(_L, "npcs");
    int len = luaL_len(_L, -1);

    for (int i = 1; i <= len; ++i)
    {
        lua_rawgeti(_L, -1, i);

        NPCSpawnInfo info;
        info.id = NPC_ID_START + (i - 1);

        lua_getfield(_L, -1, "x");           info.x = lua_tointeger(_L, -1); lua_pop(_L, 1);
        lua_getfield(_L, -1, "y");           info.y = lua_tointeger(_L, -1); lua_pop(_L, 1);
        lua_getfield(_L, -1, "name");        info.name = lua_tostring(_L, -1);  lua_pop(_L, 1);
        lua_getfield(_L, -1, "move_type");   info.moveType = ParseMoveType(lua_tostring(_L, -1));   lua_pop(_L, 1);
        lua_getfield(_L, -1, "battle_type"); info.battleType = ParseBattleType(lua_tostring(_L, -1)); lua_pop(_L, 1);
        lua_getfield(_L, -1, "level");       info.level = lua_tointeger(_L, -1); lua_pop(_L, 1);

        out.push_back(info);
        lua_pop(_L, 1);
    }
    lua_pop(_L, 1);
}

MOVE_TYPE ScriptManager::ParseMoveType(const std::string& s)
{
    if (s == "FIXED")   return MOVE_TYPE::FIXED;
    if (s == "ROAMING") return MOVE_TYPE::ROAMING;
    std::cerr << "알 수 없는 MoveType: " << s << std::endl;
    return MOVE_TYPE::ROAMING;
}

BATTLE_TYPE ScriptManager::ParseBattleType(const std::string& s)
{
    if (s == "PEACE") return BATTLE_TYPE::PEACE;
    if (s == "AGRO")  return BATTLE_TYPE::AGRO;
    std::cerr << "알 수 없는 BattleType: " << s << std::endl;
    return BATTLE_TYPE::PEACE; 
}