#pragma once

using GameEvent = std::function<void()>;

class GameLogicThread
{
public:
    GameLogicThread() = default;
    ~GameLogicThread() = default;

    void PostEvent(GameEvent event);
    void Run();
    void Stop();

private:
    std::mutex _queueMutex;
    std::queue<GameEvent> _queue;
    std::condition_variable _cv;
    bool _stopped = false;
};

extern std::shared_ptr<GameLogicThread> GGameLogicThread;