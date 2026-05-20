#include "pch.h"
#include "WorkerThread.h"
#include "Session.h"

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

        // TODO:
        // overlapped를 OverlappedEx로 캐스팅
        // IO_ACCEPT / IO_RECV / IO_SEND 분기
        // 1. 메모리 주소가 같으므로 안전하게 캐스팅
        OverlappedEx* exOver = reinterpret_cast<OverlappedEx*>(overlapped);

        // 2. Completion Key를 Session 포인터로 복원
        // (미리 CreateIoCompletionPort 호출 시 세션 포인터를 key로 넣었다고 가정)
        Session* session = reinterpret_cast<Session*>(key);

        // 3. 에러 및 연결 종료 처리
        if (FALSE == result || (bytes == 0 && (exOver->_io_type == IO_RECV || exOver->_io_type == IO_SEND)))
        {
            // TODO: 세션 종료 처리 (예: session->Disconnect())

            // Send 객체는 보통 new로 동적 할당하므로 메모리 누수 방지를 위해 해제
            if (exOver->_io_type == IO_SEND)
                delete exOver;

            continue;
        }

        // 4. IO 타입에 따른 분기 처리
        switch (exOver->_io_type)
        {
        case IO_ACCEPT:
        {
            AcceptOverlapped* acceptOver = static_cast<AcceptOverlapped*>(exOver);
            // TODO: 새 클라이언트 연결 처리
            // Session 매니저에 새 세션 등록, 새 소켓 생성, 바인딩 후 첫 Recv 걸기
            // 다시 PostAcceptRequest() 호출


            std::cout << "클라이언트 접속" << std::endl;
            break;
        }
        case IO_RECV:
        {
            RecvOverlapped* recvOver = static_cast<RecvOverlapped*>(exOver);
            // TODO: 수신된 데이터(bytes)만큼 패킷 처리 로직 (패킷 조립 및 라우팅)
            // session->OnRecv(recvOver->_recv_buf, bytes);

            // 처리가 끝나면 다시 비동기 Recv 걸기
            // recvOver->ReadyToRecv();
            // WSARecv(...);
            break;
        }
        case IO_SEND:
        {
            SendOverlapped* sendOver = static_cast<SendOverlapped*>(exOver);
            // Send는 운영체제가 복사를 마쳤으므로 객체만 정리해주면 됨
            delete sendOver;
            break;
        }
        default:
            break;
        }
    }
}
