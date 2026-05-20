#include "pch.h"
#include <iostream>
#include "WorkerThread.h"
#include "TimerThread.h"
#include "GameLogicThread.h"
#include "Session.h"
#include "NetworkManager.h"

int main()
{
    NetworkManager::Start();

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

    for (auto& th : workerThreads)
    {
        if (th.joinable())
            th.join();
    }

    NetworkManager::Stop();
}
