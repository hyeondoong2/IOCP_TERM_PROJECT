#pragma once

class Sector
{
public:
    void AddObject(int objectId)
    {
        if (objectId < MAX_PLAYERS)
        {
            _playerIds.insert(objectId);
        }
        else
        {
            _npcIds.insert(objectId);
        };

        _objectIds.insert(objectId);
    }

    void RemoveObject(int objectId)
    {
        if (objectId < MAX_PLAYERS)
        {
            _playerIds.erase(objectId);
        }
        else
        {
            _npcIds.erase(objectId);
        }
    }

    const std::unordered_set<int>& GetObjectIds() const
    {
        return _objectIds;
    }


    const std::unordered_set<int>& GetPlayerIds() const
    {
        return _playerIds;
    }

    const std::unordered_set<int>& GetNpcIds() const
    {
        return _npcIds;
    }

private:

    std::unordered_set<int> _objectIds;
    std::unordered_set<int> _playerIds;
    std::unordered_set<int> _npcIds;
};