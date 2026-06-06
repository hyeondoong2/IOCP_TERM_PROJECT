#include "pch.h"
#include "NetworkManager.h"
#include "WorkerThread.h"
#include "Session.h"
#include "SessionManager.h"

HANDLE g_hIocp = INVALID_HANDLE_VALUE;
SOCKET g_listenSocket = INVALID_SOCKET;

void NetworkManager::Init()
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    g_listenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

    // IOCP 생성
    g_hIocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);

    // listen 소켓을 IOCP에 연결 (키는 9999로 임의 설정)
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

    if (g_hIocp != NULL && g_hIocp != INVALID_HANDLE_VALUE)
    {
        ::CloseHandle(g_hIocp);
        g_hIocp = INVALID_HANDLE_VALUE;
    }

    ::WSACleanup();
}

void NetworkManager::OnAcceptComplete(AcceptOverlapped* acceptOver, std::shared_ptr<Session> session, bool success)
{
    if (acceptOver == nullptr || session == nullptr || success == false)
    {
        if (session != nullptr)
            session->Disconnect();

        delete acceptOver;
        PostAccept();
        return;
    }

    const int id = GSessionManager->Add(session);
    if (id == -1)
    {
        std::cout << "SessionManager is full. Accept rejected.\n";
        session->Disconnect();
        delete acceptOver;
        PostAccept();
        return;
    }

    ::setsockopt(session->_socket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
        reinterpret_cast<char*>(&g_listenSocket), sizeof(g_listenSocket));

    session->SetId(id);
    session->_st = ST_ALLOC;

    HANDLE result = ::CreateIoCompletionPort(
        reinterpret_cast<HANDLE>(session->_socket),
        g_hIocp,
        static_cast<ULONG_PTR>(id),
        0);

    if (result == NULL)
    {
        std::cout << "CreateIoCompletionPort(client) Error: " << ::GetLastError() << "\n";
        session->Disconnect();
        delete acceptOver;
        PostAccept();
        return;
    }

    std::cout << "Client Accepted. SessionId: " << id << "\n";

    session->DoRecv();

    delete acceptOver;
    PostAccept();
}

// Accept를 위한 낚시대!
void NetworkManager::PostAccept()
{
    std::shared_ptr<Session> new_session = std::make_shared<Session>();

    new_session->_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (new_session->_socket == INVALID_SOCKET)
    {
        return;
    }

    AcceptOverlapped* accept_over = new AcceptOverlapped(new_session);

    DWORD bytes = 0;
    int addrSize = sizeof(SOCKADDR_IN);

    BOOL ret = AcceptEx(
        g_listenSocket,
        new_session->_socket,
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
            new_session->Disconnect();
            delete accept_over;
            std::cout << "AcceptEx Error: " << err << "\n";
        }
    }
}

