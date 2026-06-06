#pragma once

class GameObject;

class ObjectManager
{
public:
    void AddObject(std::shared_ptr<GameObject> obj);
    void RemoveObject(int objectId);
    std::shared_ptr<GameObject> FindObject(int objectId);

    template<typename T>
    std::shared_ptr<T> FindAs(int objectId)
    {
        auto obj = FindObject(objectId);
        if (!obj) return nullptr;

        return std::dynamic_pointer_cast<T>(obj);
    }

private:
    std::mutex _mutex;
    std::unordered_map<int, std::shared_ptr<GameObject>> _objects;
};

extern std::shared_ptr<ObjectManager> GObjectManager;