#include "pch.h"
#include "ObjectSpawner.h"
#include "NPC.h"
#include "ObjectManager.h"
#include "SectorManager.h"
#include "TimerThread.h"
#include "Collision.h"

std::shared_ptr<ObjectSpawner> GObjectSpawner = std::make_shared<ObjectSpawner>();

void ObjectSpawner::Init()
{
    lua_State* L = luaL_newstate();
    luaL_openlibs(L);

    if (luaL_dofile(L, "npc_data.lua") != LUA_OK)
    {
        std::cerr << "Lua 파일 로드 실패: " << lua_tostring(L, -1) << std::endl;
        lua_close(L);
        return;
    }

    lua_getglobal(L, "npc_types");
    if (!lua_istable(L, -1))
    {
        std::cerr << "npc_types 테이블을 찾을 수 없습니다." << std::endl;
        lua_close(L);
        return;
    }

    int totalSpawned = 0;

    lua_pushnil(L);
    while (lua_next(L, -2) != 0)
    {
        if (lua_istable(L, -1))
        {
            lua_getfield(L, -1, "type");
            std::string type = lua_tostring(L, -1);
            lua_pop(L, 1);

            lua_getfield(L, -1, "move_type");
            std::string moveStr = lua_tostring(L, -1);
            lua_pop(L, 1);

            lua_getfield(L, -1, "battle_type");
            std::string battleStr = lua_tostring(L, -1);
            lua_pop(L, 1);

            lua_getfield(L, -1, "level");
            int level = lua_tointeger(L, -1);
            lua_pop(L, 1);

            lua_getfield(L, -1, "count");
            int count = lua_tointeger(L, -1);
            lua_pop(L, 1);

            MOVE_TYPE moveType = (moveStr == "FIXED") ? MOVE_TYPE::FIXED : MOVE_TYPE::ROAMING;
            BATTLE_TYPE battleType = (battleStr == "AGRO") ? BATTLE_TYPE::AGRO : BATTLE_TYPE::PEACE;


            int startId = NPC_ID_START;
            if (type == "BLUESLIME")      startId = BLUE_SLIME_ID_START;
            else if (type == "CHICKEN")   startId = CHICKEN_ID_START;
            else if (type == "COW")       startId = COW_ID_START;
            else if (type == "REDSLIME")  startId = RED_SLIME_ID_START;
            else
            {
                std::cout << "알 수 없는 몬스터 타입입니다: " << type << std::endl;
            }

            for (int i = 0; i < count; ++i)
            {
                short x = 0, y = 0;

                do
                {
                    x = rand() % WORLD_WIDTH;
                    y = rand() % WORLD_HEIGHT;
                }
                while (isCollision(x, y));

                std::string name = type + "_" + std::to_string(i);

                int npcId = startId + i;
                SpawnNPC(npcId, x, y, name, moveType, battleType, level);

                totalSpawned++;
            }
        }
        lua_pop(L, 1);
    }

    lua_close(L);
    std::cout << "NPC 스포닝 완료! (총 " << totalSpawned << "마리)" << std::endl;
}

std::shared_ptr<NPC> ObjectSpawner::SpawnNPC(int id, short x, short y, const std::string& name,
    MOVE_TYPE moveType, BATTLE_TYPE battleType, int level)
{
    auto npc = std::make_shared<NPC>();

    npc->Init(id, x, y, name, moveType, battleType, level);

    GObjectManager->AddObject(npc);
    GSectorManager->AddObject(npc);

    return npc;
}
void ObjectSpawner::SpawnFromLua(std::string type, int x, int y)
{
}
