#include "pch.h"
#include "ObjectSpawner.h"
#include "NPC.h"
#include "ObjectManager.h"
#include "SectorManager.h"

void ObjectSpawner::Init()
{
    std::cout << "NPC 스포닝 시작 (" << NUM_NPCS << " 마리)..." << std::endl;

    for (int i = 0; i < NUM_NPCS; ++i)
    {
        int id = NPC_ID_START + i;
        short x = rand() % WORLD_WIDTH;
        short y = rand() % WORLD_HEIGHT;

        std::string name = "NPC_" + std::to_string(i);

        SpawnNPC(id, x, y, name);
    }

    std::cout << "NPC 스포닝 완료." << std::endl;
}

std::shared_ptr<NPC> ObjectSpawner::SpawnNPC(int id, short x, short y, std::string name)
{
    auto npc = std::make_shared<NPC>();
    npc->Init(id, x, y, name);
    GObjectManager->AddObject(npc);
    GSectorManager->AddObject(npc);
    return npc;
}

//std::shared_ptr<Monster> ObjectSpawner::SpawnMonster(int id, short x, short y, std::string name, int level)
//{
//    auto monster = std::make_shared<Monster>();
//    //monster->Init(id, x, y, name); 
//    //monster->_level = level;     
//    GObjectManager->AddObject(monster);
//    return monster;
//}

void ObjectSpawner::SpawnFromLua(std::string type, int x, int y)
{
}
