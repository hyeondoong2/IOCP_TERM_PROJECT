#pragma once

class NPC;

class ObjectSpawner
{
public:
    void Init();

    // NPC 积己
    static std::shared_ptr<NPC> SpawnNPC(int id, short x, short y, std::string name);

    // Lua 胶农赋飘 扁馆 积己 
    static void SpawnFromLua(std::string type, int x, int y);
};

extern std::shared_ptr<ObjectSpawner> GObjectSpawner;