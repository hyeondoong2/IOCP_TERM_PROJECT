#pragma once

#include <memory>

extern HANDLE g_hIocp;
extern SOCKET g_listenSocket;

class AcceptOverlapped;
class Session;

class NetworkManager
{
private:
    static void Init();
    static void BindAndListen();
    static void PostAccept();

public:
    NetworkManager() = default;
    ~NetworkManager()
    {
        Stop();
    }

    static void Start();
    static void Stop();
    static void OnAcceptComplete(AcceptOverlapped* acceptOver, std::shared_ptr<Session> session, bool success);

    static HANDLE GetIocpHandle() { return g_hIocp; }
};

