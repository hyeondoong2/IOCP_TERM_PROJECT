#include "pch.h"
#include <iostream>
#include "WorkerThread.h"
#include "TimerThread.h"
#include "GameLogicThread.h"
#include "Session.h"
#include "NetworkManager.h"
#include "DBThread.h"
#include "ObjectSpawner.h"
#include "Collision.h"

namespace
{
    constexpr size_t DB_CONNECTION_COUNT = 8;
    const std::wstring DB_CONNECTION_STRING = L"DSN=GameServer_DSN;";
}

int main()
{
    NetworkManager::Start();

    InitCollisionTile();

    // DB ÃÊ±âÈ­
    if(!GDBManager->Init(DB_CONNECTION_COUNT, DB_CONNECTION_STRING))
    {
        std::cerr << "Failed to initialize DB Manager.\n";
        return -1;
    }

    HANDLE hIocp = NetworkManager::GetIocpHandle();

    // worker thread
    unsigned int workerCount = std::thread::hardware_concurrency();

    std::vector<std::thread> workerThreads;

    for (unsigned int i = 0; i < workerCount; ++i)
    {
        workerThreads.emplace_back([hIocp]()
            {
                WorkerThread worker;
                worker.DoWork(hIocp);
            });
    }

    // DB Thread
    std::vector<std::thread> dbThreads;
    for (size_t i = 0; i < DB_CONNECTION_COUNT; ++i)
    {
        dbThreads.emplace_back([]()
        {
            GDBManager->Run();
        });
    }

    // game logic thread
    std::thread logicThread([]() { GGameLogicThread->Run(); });

    // timer thread
    std::thread timerThread([]() { GTimerThread->RunTimer(); });

    GGameLogicThread->PostEvent([]()
        {
            GObjectSpawner->Init(); // spawn npc
        });

    for (auto& th : workerThreads)
    {
        if (th.joinable())
            th.join();
    }

    NetworkManager::Stop();

    GGameLogicThread->Stop();
    if (logicThread.joinable())
        logicThread.join();

    GTimerThread->Stop();
    if (timerThread.joinable())
        timerThread.join();

    GDBManager->Stop();
    for (auto& th : dbThreads)
    {
        if (th.joinable())
            th.join();
    }


}
