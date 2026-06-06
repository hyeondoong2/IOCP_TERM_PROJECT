#pragma once

#include <memory>
#include <mutex>
#include <unordered_map>

class Session;

class SessionManager
{
public:
    SessionManager()
    {
        // create id pool
        _freeIds.reserve(MAX_PLAYERS);
        for (int i = MAX_PLAYERS - 1; i >= 0; --i)
        {
            _freeIds.push_back(i);
        }
    }
    ~SessionManager() = default;

    int Add(std::shared_ptr<Session> session);

    std::shared_ptr<Session> Find(int id);

    void Remove(int id);
    void Clear();

private:
    int IssueId();

private:
    std::mutex _mutex;
    std::vector<int> _freeIds;
    std::unordered_map<int, std::shared_ptr<Session>> _sessions;
};

extern std::shared_ptr<SessionManager> GSessionManager;

