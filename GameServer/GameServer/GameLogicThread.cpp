#include "pch.h"
#include "GameLogicThread.h"

std::shared_ptr<GameLogicThread> GGameLogicThread = std::make_shared<GameLogicThread>();

void GameLogicThread::PostEvent(GameEvent event)
{
    {
        std::lock_guard<std::mutex> lock(_queueMutex);

        // 복사하지 않고 move semantics로 이벤트를 큐에 넣음
        _queue.push(std::move(event));
    }

    _cv.notify_one();
}

void GameLogicThread::Run()
{
    while (true)
    {
        std::queue<GameEvent> localQueue;

        {
            std::unique_lock<std::mutex> lock(_queueMutex);

            _cv.wait(lock, [this]()
                {
                    return !_queue.empty() || _stopped;
                });

            if (_stopped && _queue.empty())
                return;

            std::swap(localQueue, _queue);
        } 

        while (!localQueue.empty())
        {
            auto& event = localQueue.front();
            event();
            localQueue.pop();
        }
    }
}
void GameLogicThread::Stop()
{
    {
        std::lock_guard<std::mutex> lock(_queueMutex);
        _stopped = true;
    }

    _cv.notify_all();
}
