#pragma once

extern HANDLE g_hIocp;
extern SOCKET g_listenSocket;

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

    static HANDLE GetIocpHandle() { return g_hIocp; }
};

