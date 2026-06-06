#include "pch.h"
#include "ObjectManager.h" 
#include "GameObject.h"

std::shared_ptr<ObjectManager> GObjectManager = std::make_shared<ObjectManager>();

void ObjectManager::AddObject(std::shared_ptr<GameObject> obj)
{
    if (!obj) return;
    std::lock_guard<std::mutex> lock(_mutex);
    _objects[obj->_id] = obj;
}

void ObjectManager::RemoveObject(int objectId)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _objects.erase(objectId);
}

std::shared_ptr<GameObject> ObjectManager::FindObject(int objectId)
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _objects.find(objectId);
    if (it == _objects.end()) return nullptr;
    return it->second;
}