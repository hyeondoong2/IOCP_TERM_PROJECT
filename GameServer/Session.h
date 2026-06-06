#pragma once

class Session;

class OverlappedEx
{
public:
    WSAOVERLAPPED _over;
    IO_TYPE _io_type;

    // 비동기 작업이 끝날 때까지 세션이 살아있도록 shared_ptr로 참조 유지
    std::shared_ptr<Session> _session;

public:
    explicit OverlappedEx(IO_TYPE io_type, std::shared_ptr<Session> session)
        : _io_type(io_type), _session(session)
    {
        ZeroMemory(&_over, sizeof(_over));
    }

    void Reset()
    {
        ZeroMemory(&_over, sizeof(_over));
    }
};

class AcceptOverlapped : public OverlappedEx
{
public:
    char _accept_buf[BUF_SIZE];

public:
    AcceptOverlapped(std::shared_ptr<Session> new_session)
        : OverlappedEx(IO_ACCEPT, new_session)
    {
        ZeroMemory(_accept_buf, sizeof(_accept_buf));
    }
};

class RecvOverlapped : public OverlappedEx
{
public:
    WSABUF _wsabuf;
    char _recv_buf[BUF_SIZE];

public:
    // 생성자에서는 일단 세션을 nullptr로 둔다.
    // 세션이 막 만들어지는 시점에는 shared_from_this()를 사용할 수 없다.
    RecvOverlapped() : OverlappedEx(IO_RECV, nullptr)
    {
        _wsabuf.buf = _recv_buf;
        _wsabuf.len = BUF_SIZE;
    }

    // WSARecv 직전에 호출한다.
    void ReadyToRecv(std::shared_ptr<Session> session, int prevRemain)
    {
        Reset();
        _session = session;

        _wsabuf.buf = _recv_buf + prevRemain;
        _wsabuf.len = BUF_SIZE - prevRemain;
    }
};

class SendOverlapped : public OverlappedEx
{
public:
    WSABUF _wsabuf;
    char _send_buf[BUF_SIZE];

public:
    SendOverlapped(std::shared_ptr<Session> session, const char* packet)
        : OverlappedEx(IO_SEND, session)
    {
        _wsabuf.len = static_cast<unsigned char>(packet[0]);
        _wsabuf.buf = _send_buf;
        memcpy(_send_buf, packet, _wsabuf.len);
    }
};

class Player;

// 클래스 내부에서 자기 자신의 shared_ptr를 얻을 수 있도록 enable_shared_from_this 상속
class Session : public std::enable_shared_from_this<Session>
{
public:
    int _id = -1;
    SOCKET _socket = INVALID_SOCKET;
    SOCKET_STATE _st = SOCKET_STATE::ST_FREE;
    std::mutex _mutex;
    std::weak_ptr<Player> _owner;

    RecvOverlapped _recvOverlapped;
    std::mutex _sendMutex;
    std::queue<SendOverlapped*> _sendQueue;
    int _prevRemain = 0;

public:
    Session() = default;

    explicit Session(SOCKET socket);
    ~Session();

    void SetId(int id);
    int GetId() const;

    void Disconnect();
    void DoRecv();
    void DoSend(const char* packet);

    void OnRecv(DWORD bytes);
    void OnSend(SendOverlapped* sendOver);

    // 게임 로직 스레드에서 호출할 패킷 전송 함수들
    void send_login_fail_packet();
    void send_login_success_packet();
    void send_avatar_info_packet();


private:
    bool PostSend(SendOverlapped* sendOver);
    void ProcessPacket(char* packet);
};

