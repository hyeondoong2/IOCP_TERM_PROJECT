#pragma once

#include "DBConnection.h"

class DBConnectionPool
{
public:
    DBConnectionPool() = default;
    ~DBConnectionPool();

    bool Init(size_t connectionCount, const std::wstring& connectionString);
    void Clear();
    void Stop();

    std::shared_ptr<DBConnection> Pop();
    void Push(std::shared_ptr<DBConnection> connection);

    size_t GetConnectionCount() const;

private:
    mutable std::mutex _mutex;
    std::condition_variable _cv;

    std::vector<std::shared_ptr<DBConnection>> _connections;    // All connections
    std::queue<std::shared_ptr<DBConnection>> _availableConnections;    // Available connections

    bool _stopped = false;
};
