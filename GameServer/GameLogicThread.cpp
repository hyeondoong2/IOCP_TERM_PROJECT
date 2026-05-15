#include "pch.h"
#include "GameLogicThread.h"

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
    while(true)
    {
        GameEvent event;
        {
            std::unique_lock<std::mutex> lock(_queueMutex);

            // 이벤트가 들어올 때까지 대기
            _cv.wait(lock, [this]()
                {
                    return !_queue.empty() || _stopped;
                });

            if (_stopped && _queue.empty())
                return;

            event = std::move(_queue.front());
            _queue.pop();
        }

        // 이벤트 처리
        event();
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
