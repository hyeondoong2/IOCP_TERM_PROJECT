#include "pch.h"
#include "DBConnectionPool.h"

DBConnectionPool::~DBConnectionPool()
{
    Stop();
    Clear();
}

bool DBConnectionPool::Init(size_t connectionCount, const std::wstring& connectionString)
{
    if (connectionCount == 0)
        return false;

    std::lock_guard<std::mutex> lock(_mutex);

    if (!_connections.empty())
        return true;

    _stopped = false;

    for (size_t i = 0; i < connectionCount; ++i)
    {
        std::shared_ptr<DBConnection> connection = std::make_shared<DBConnection>(static_cast<int>(i));
        if (!connection->Connect(connectionString))
        {
            while (!_availableConnections.empty())
                _availableConnections.pop();

            for (std::shared_ptr<DBConnection>& connectedConnection : _connections)
            {
                if (connectedConnection != nullptr)
                    connectedConnection->Disconnect();
            }

            _connections.clear();
            return false;
        }

        _connections.push_back(connection);
        _availableConnections.push(connection);
    }

    return true;
}

void DBConnectionPool::Clear()
{
    std::lock_guard<std::mutex> lock(_mutex);

    while (!_availableConnections.empty())
        _availableConnections.pop();

    for (std::shared_ptr<DBConnection>& connection : _connections)
    {
        if (connection != nullptr)
            connection->Disconnect();
    }

    _connections.clear();
}

void DBConnectionPool::Stop()
{
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _stopped = true;
    }

    _cv.notify_all();
}

std::shared_ptr<DBConnection> DBConnectionPool::Pop()
{
    std::unique_lock<std::mutex> lock(_mutex);

    _cv.wait(lock, [this]()
        {
            return _stopped || !_availableConnections.empty();
        });

    if (_stopped)
        return nullptr;

    std::shared_ptr<DBConnection> connection = _availableConnections.front();
    _availableConnections.pop();
    return connection;
}

void DBConnectionPool::Push(std::shared_ptr<DBConnection> connection)
{
    if (connection == nullptr)
        return;

    {
        std::lock_guard<std::mutex> lock(_mutex);

        if (_stopped)
            return;

        _availableConnections.push(connection);
    }

    _cv.notify_one();
}

size_t DBConnectionPool::GetConnectionCount() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _connections.size();
}
