#include "pch.h"
#include "NetworkManager.h"
#include "WorkerThread.h"
#include "Session.h"

bool NetworkManager::Init()
{

}

void NetworkManager::Stop()
{
    if (_listenSocket != INVALID_SOCKET)
    {
        ::closesocket(_listenSocket);
        _listenSocket = INVALID_SOCKET;
    }

    if (_hIocp != INVALID_HANDLE_VALUE)
    {
        ::CloseHandle(_hIocp);
        _hIocp = INVALID_HANDLE_VALUE;
    }

    ::WSACleanup();
}
