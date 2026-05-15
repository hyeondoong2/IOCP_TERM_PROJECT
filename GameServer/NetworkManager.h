#pragma once

class NetworkManager
{
private:
    HANDLE _hIocp;
    SOCKET _listenSocket;
    SOCKET _clientSocket;

public:
    NetworkManager() = default;
    ~NetworkManager()
    {
        Stop();
    }

    bool Init();
    bool Listen();                
    void Stop();
    bool PostAccept();;

    HANDLE GetIocpHandle() { return _hIocp; }
};