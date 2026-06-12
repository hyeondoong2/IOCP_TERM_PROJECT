#include "pch.h"
#include "ObjectManager.h" 
#include "GameObject.h"
#include "Player.h"

std::shared_ptr<ObjectManager> GObjectManager = std::make_shared<ObjectManager>();

void ObjectManager::AddObject(std::shared_ptr<GameObject> obj)
{
    if (!obj) return;
    _objects[obj->_id] = obj;
}

void ObjectManager::RemoveObject(int objectId)
{
    _objects.erase(objectId);
}

std::shared_ptr<GameObject> ObjectManager::FindObject(int objectId)
{
    auto it = _objects.find(objectId);
    if (it == _objects.end()) return nullptr;
    return it->second;
}

bool ObjectManager::IsPlayerNameInUse(const std::string& name)
{
    return _activePlayerNames.count(name) > 0;
}
