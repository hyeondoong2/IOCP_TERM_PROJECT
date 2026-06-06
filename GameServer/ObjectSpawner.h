#pragma once

class NPC;

class ObjectSpawner
{
public:
    void Init();

    // NPC 생성
    static std::shared_ptr<NPC> SpawnNPC(int id, short x, short y, std::string name);

    // 몬스터 생성
    //static std::shared_ptr<Monster> SpawnMonster(int id, short x, short y, std::string name, int level);

    // Lua 스크립트 기반 생성 
    static void SpawnFromLua(std::string type, int x, int y);
};