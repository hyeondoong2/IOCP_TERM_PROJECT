#pragma once

class GameObject;

class ObjectManager
{
public:
    void AddObject(std::shared_ptr<GameObject> obj);
    void RemoveObject(int objectId);
    
    std::shared_ptr<GameObject> FindObject(int objectId);

    bool IsPlayerNameInUse(const std::string& name);

    void AddPlayerName(const std::string& name) { _activePlayerNames.insert(name); }
    void RemovePlayerName(const std::string& name) { _activePlayerNames.erase(name); }

private:
    std::unordered_map<int, std::shared_ptr<GameObject>> _objects;
    std::unordered_set<std::string> _activePlayerNames;
};

extern std::shared_ptr<ObjectManager> GObjectManager;