#include "pch.h"
#include "DBThread.h"

std::shared_ptr<DBThread> GDBManager = std::make_shared<DBThread>();

DBThread::~DBThread()
{
    Stop();
    Clear();
}

bool DBThread::Init(size_t connectionCount, const std::wstring& connectionString)
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (_initialized)
        return true;

    if (!_connectionPool.Init(connectionCount, connectionString))
        return false;

    _stopped = false;
    _initialized = true;
    return true;
}

void DBThread::Clear()
{
    _connectionPool.Clear();

    std::lock_guard<std::mutex> lock(_mutex);

    while (!_queue.empty())
        _queue.pop();

    _initialized = false;
}

void DBThread::PostTask(DBTask task)
{
    {
        std::lock_guard<std::mutex> lock(_mutex);

        if (_stopped || !_initialized)
            return;

        _queue.push(std::move(task));
    }

    _cv.notify_one();
}

void DBThread::Run()
{
    while (true)
    {
        DBTask task;

        {
            std::unique_lock<std::mutex> lock(_mutex);

            _cv.wait(lock, [this]()
                {
                    return _stopped || !_queue.empty();
                });

            if (_stopped && _queue.empty())
                return;

            task = std::move(_queue.front());
            _queue.pop();
        }

        // 커넥션 풀에서 커넥션을 가져와서 작업 수행
        std::shared_ptr<DBConnection> connection = _connectionPool.Pop();
        if (connection == nullptr)
            continue;

        try
        {
            if (task)
                task(*connection);
        }
        catch (...)
        {
            std::cout << "DBTask Exception\n";
        }

        // 작업이 끝난 커넥션을 다시 풀에 반환
        _connectionPool.Push(connection);
    }
}

void DBThread::Stop()
{
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _stopped = true;
    }

    _connectionPool.Stop();
    _cv.notify_all();
}

bool DBThread::IsRunning() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return !_stopped && _initialized;
}
