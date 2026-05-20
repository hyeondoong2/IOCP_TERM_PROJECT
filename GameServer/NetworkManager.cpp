#include "pch.h"
#include "NetworkManager.h"
#include "WorkerThread.h"
#include "Session.h"

HANDLE g_hIocp = NULL;
SOCKET g_listenSocket = NULL;

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

void NetworkManager::Start()
{
    Init();
    BindAndListen();
    PostAccept();
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
    // 1. 앞으로 접속할 손님을 앉힐 '빈 세션'을 미리 생성합니다.
    std::shared_ptr<Session> new_session = std::make_shared<Session>();

    // 2. 세션의 소켓을 미리 생성해서 넣어둡니다.
    new_session->_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (new_session->_socket == INVALID_SOCKET)
    {
        // 소켓 생성 실패 시 그냥 리턴하면 new_session은 자동 소멸됩니다.
        return;
    }

    // 3. AcceptOverlapped를 동적 할당하고, 미리 만든 세션을 뱃속에 쥐어줍니다.
    AcceptOverlapped* accept_over = new AcceptOverlapped(new_session);

    DWORD bytes = 0;
    int addrSize = sizeof(SOCKADDR_IN);

    // 4. AcceptEx 호출 (listen 소켓과 방금 만든 세션의 소켓을 엮어줍니다)
    BOOL ret = AcceptEx(
        g_listenSocket,
        new_session->_socket,       // <-- 전역 변수 대신 세션의 소켓 사용!
        accept_over->_accept_buf,
        0,
        addrSize + 16,
        addrSize + 16,
        &bytes,
        &accept_over->_over
    );

    if (ret == FALSE)
    {
        int err = WSAGetLastError();
        if (err != WSA_IO_PENDING)
        {
            delete accept_over;
            std::cout << "AcceptEx Error: " << err << "\n";
        }
    }
}
