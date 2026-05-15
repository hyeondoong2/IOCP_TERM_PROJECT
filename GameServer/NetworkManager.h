#pragma once

class AcceptOverlapped;

extern HANDLE g_hIocp;
extern SOCKET g_listenSocket;
extern SOCKET g_acceptSocket;

class NetworkManager
{
private:
    static AcceptOverlapped _acceptContext;

    static void Init();
    static void BindAndListen();
    static void PostAccept();
public:
    NetworkManager() = default;
    ~NetworkManager()
    {
        Stop();
    }

    static bool Start();
    static void Stop();

    HANDLE GetIocpHandle() { return g_hIocp; }
};

