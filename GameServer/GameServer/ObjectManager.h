#pragma once

class GameObject;

class ObjectManager
{
public:
    void AddObject(std::shared_ptr<GameObject> obj);
    void RemoveObject(int objectId);
    
    std::shared_ptr<GameObject> FindObject(int objectId);

    bool IsPlayerNameInUse(const std::string& name);
private:
    std::mutex _mutex;
    std::unordered_map<int, std::shared_ptr<GameObject>> _objects;
};

extern std::shared_ptr<ObjectManager> GObjectManager;