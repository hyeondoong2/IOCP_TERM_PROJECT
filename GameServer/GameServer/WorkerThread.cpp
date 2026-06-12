#include "pch.h"
#include "WorkerThread.h"
#include "Session.h"
#include "SessionManager.h"
#include "NetworkManager.h"

void WorkerThread::Disconnect(int clientId)
{
    std::shared_ptr<Session> session = GSessionManager->Find(clientId);
    if (session != nullptr)
        session->Disconnect();
}

void WorkerThread::DoWork(HANDLE hIocp)
{
    while (true)
    {
        DWORD bytes = 0;
        ULONG_PTR key = 0;
        WSAOVERLAPPED* overlapped = nullptr;

        BOOL result = GetQueuedCompletionStatus(
            hIocp,
            &bytes,
            &key,
            &overlapped,
            INFINITE);

        if (overlapped == nullptr)
            break;

        OverlappedEx* exOver = reinterpret_cast<OverlappedEx*>(overlapped);

        std::shared_ptr<Session> session = exOver->_session;

        // Overlapped 객체가 완료된 뒤에도 Session을 계속 붙잡지 않도록 비움
        exOver->_session.reset();

        if (FALSE == result)
        {
            const int err = ::WSAGetLastError();
            //std::cout << "GQCS Error: " << err << "\n";

            if (exOver->_io_type == IO_ACCEPT)
            {
                NetworkManager::OnAcceptComplete(static_cast<AcceptOverlapped*>(exOver), session, false);
            }
            else
            {
                if (session != nullptr)
                    session->Disconnect();

                if (exOver->_io_type == IO_SEND)
                {
                    SendOverlapped* sendOver = static_cast<SendOverlapped*>(exOver);
                    if (session != nullptr)
                        session->OnSend(sendOver);
                    else
                        delete sendOver;
                }
            }

            continue;
        }

        if (bytes == 0 && exOver->_io_type == IO_RECV)
        {
            if (session != nullptr)
                session->Disconnect();

            continue;
        }

        if (bytes == 0 && exOver->_io_type == IO_SEND)
        {
            if (session != nullptr)
            {
                session->Disconnect();
                session->OnSend(static_cast<SendOverlapped*>(exOver));
            }
            else
            {
                delete static_cast<SendOverlapped*>(exOver);
            }

            continue;
        }

        switch (exOver->_io_type)
        {
        case IO_ACCEPT:
        {
            AcceptOverlapped* acceptOver = static_cast<AcceptOverlapped*>(exOver);
            NetworkManager::OnAcceptComplete(acceptOver, session, true);
            break;
        }
        case IO_RECV:
        {
            if (session != nullptr)
                session->OnRecv(bytes);
            break;
        }
        case IO_SEND:
        {
            SendOverlapped* sendOver = static_cast<SendOverlapped*>(exOver);

            if (session != nullptr)
                session->OnSend(sendOver);
            else
                delete sendOver;

            break;
        }
        default:
            break;
        }
    }
}

