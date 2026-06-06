#pragma once

#define WIN32_LEAN_AND_MEAN

// C++ standard headers
#include <iostream>
#include <array>
#include <thread>
#include <vector>
#include <mutex>
#include <functional>
#include <condition_variable>
#include <memory>
#include <unordered_set>
#include <chrono>
#include <atomic>
#include <cstring>
#include <queue>
#include <algorithm>
#include <ranges>
#include <string>
#include <concurrent_priority_queue.h>
#include <tbb/concurrent_unordered_map.h>

// Windows headers
#include <WS2tcpip.h>
#include <MSWSock.h>
#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "MSWSock.lib")
#pragma comment(lib, "odbc32.lib")
#include <windows.h>

// GameServer headers
#include "protocol_2026.h"
#include "Global.h"

// Database headers
#include <sql.h>
#include <sqlext.h>

// Timer 
enum TIMER_EVENT_TYPE
{
    TIMER_EVENT_MOVE,
    TIMER_EVENT_NPC_MOVE,
};

struct TIMER_EVENT
{
    int obj_id;
    std::chrono::steady_clock::time_point wakeup_time;
    TIMER_EVENT_TYPE event_type;
    int target_id;
    uint64_t sequence = 0;
};

inline uint32_t GetNowTime()
{
    return static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

// IO
enum IO_TYPE
{
    IO_ACCEPT,
    IO_RECV,
    IO_SEND,
};

// Socket
enum SOCKET_STATE 
{ 
    ST_FREE, 
    ST_ALLOC, 
    ST_INGAME 
};

// Player
enum class PLAYER_STATE
{
    NONE = 0,
    LOBBY,      // 로비 대기 중
    IN_GAME,    // 게임 월드 접속 완료
    DEAD        // 사망 상태 
};