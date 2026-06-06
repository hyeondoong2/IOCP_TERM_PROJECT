#pragma once

class Sector
{
public:
    void AddObject(int objectId)
    {
        _objects.insert(objectId);
    }

    void RemoveObject(int objectId)
    {
        _objects.erase(objectId);
    }

    const std::unordered_set<int>& GetObjects() const
    {
        return _objects;
    }

private:
    std::unordered_set<int> _objects;
};