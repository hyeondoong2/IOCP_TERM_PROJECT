#include "pch.h"
#include "NetworkManager.h"
#include "WorkerThread.h"
#include "Session.h"

HANDLE g_hIocp = NULL;
SOCKET g_listenSocket = NULL;
SOCKET g_acceptSocket = NULL;

AcceptOverlapped NetworkManager::_acceptContext;

void NetworkManager::Init()
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    g_listenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

    g_hIocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);

    CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_listenSocket), g_hIocp, 9999, 0);
}

void NetworkManager::BindAndListen()
{
    SOCKADDR_IN addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    ::bind(g_listenSocket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    ::listen(g_listenSocket, SOMAXCONN);
}

bool NetworkManager::Start()
{
    Init();
    BindAndListen();
    PostAccept();
    return true;
}

void NetworkManager::Stop()
{
    if (g_listenSocket != INVALID_SOCKET)
    {
        ::closesocket(g_listenSocket);
        g_listenSocket = INVALID_SOCKET;
    }

    if (g_hIocp != INVALID_HANDLE_VALUE)
    {
        ::CloseHandle(g_hIocp);
        g_hIocp = INVALID_HANDLE_VALUE;
    }

    ::WSACleanup();
}

void NetworkManager::PostAccept()
{
    if (g_acceptSocket != INVALID_SOCKET)
    {
        closesocket(g_acceptSocket);
        g_acceptSocket = INVALID_SOCKET;
    }

    g_acceptSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (g_acceptSocket == INVALID_SOCKET)
        return;

    _acceptContext.Reset();

    DWORD bytes = 0;
    int addrSize = sizeof(SOCKADDR_IN);

    AcceptEx(
        g_listenSocket,
        g_acceptSocket,
        _acceptContext._accept_buf,
        0,
        addrSize + 16,
        addrSize + 16,
        &bytes,
        &_acceptContext._over
    );
}
