#include "pch.h"
#include "SessionManager.h"
#include "Session.h"

std::shared_ptr<SessionManager> GSessionManager = std::make_shared<SessionManager>();

int SessionManager::Add(std::shared_ptr<Session> session)
{
    if (session == nullptr)
        return -1;

    std::lock_guard<std::mutex> lock(_mutex);

    const int id = IssueId();
    if (id == -1)
        return -1;

    _sessions[id] = session;
    return id;
}

std::shared_ptr<Session> SessionManager::Find(int id)
{
    std::lock_guard<std::mutex> lock(_mutex);

    auto it = _sessions.find(id);
    if (it == _sessions.end())
        return nullptr;

    return it->second;
}

void SessionManager::Remove(int id)
{
    std::shared_ptr<Session> sessionToRelease;
    {
        std::lock_guard<std::mutex> lock(_mutex);

        auto it = _sessions.find(id);
        if (it != _sessions.end())
        {
            sessionToRelease = it->second;
            _sessions.erase(it);

            _freeIds.push_back(id);
        }
    }
}

void SessionManager::Clear()
{
    std::lock_guard<std::mutex> lock(_mutex);
    _sessions.clear();
}

int SessionManager::IssueId()
{
    if (_freeIds.empty())
        return -1;

    int id = _freeIds.back();
    _freeIds.pop_back();    

    return id;
}

