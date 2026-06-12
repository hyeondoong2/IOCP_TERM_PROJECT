#pragma once

#include "DBConnectionPool.h"

using DBTask = std::function<void(DBConnection&)>;

class DBThread
{
public:
    DBThread() = default;
    ~DBThread();

    bool Init(size_t connectionCount, const std::wstring& connectionString);
    void Clear();

    void PostTask(DBTask task);
    void Run();
    void Stop();

    bool IsRunning() const;

private:
    mutable std::mutex _mutex;
    std::condition_variable _cv;
    std::queue<DBTask> _queue;

    DBConnectionPool _connectionPool;

    bool _stopped = false;
    bool _initialized = false;
};

extern std::shared_ptr<DBThread> GDBManager;
