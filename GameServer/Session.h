#pragma once

class OverlappedEx
{
public:
    WSAOVERLAPPED _over;
    IO_TYPE _io_type;

public:
    explicit OverlappedEx(IO_TYPE io_type) : _io_type(io_type)
    {
        Reset();
    }

    void Reset()
    {
        ZeroMemory(&_over, sizeof(_over));
    }
};

class AcceptOverlapped : public OverlappedEx
{
public:
    AcceptOverlapped() : OverlappedEx(IO_ACCEPT)
    {
    }
};

class RecvOverlapped : public OverlappedEx
{
public:
    WSABUF _wsabuf;
    char _recv_buf[BUF_SIZE];

public:
    RecvOverlapped() : OverlappedEx(IO_RECV)
    {
        _wsabuf.buf = _recv_buf;
        _wsabuf.len = BUF_SIZE;
    }

    void ReadyToRecv()
    {
        Reset();
    }
};

class SendOverlapped : public OverlappedEx
{
public:
    WSABUF _wsabuf;
    char _send_buf[BUF_SIZE];

public:
    SendOverlapped(char* packet) : OverlappedEx(IO_SEND)
    {
        _wsabuf.len = packet[0];
        _wsabuf.buf = _send_buf;
        memcpy(_send_buf, packet, packet[0]);
    }
};

class Player;

// 클래스 내부에서 자기 자신의 shared_ptr를 얻을 수 있도록 enable_shared_from_this 상속
class Session : public std::enable_shared_from_this<Session>
{
public:
    SOCKET _socket = INVALID_SOCKET;
    SOCKET_STATE _st = SOCKET_STATE::ST_FREE;
    std::mutex _mutex;
    std::weak_ptr<Player> _owner;

    RecvOverlapped _recvOverlapped;
    std::queue<SendOverlapped*> _sendQueue;

public:
    explicit Session(SOCKET socket);
    ~Session() = default;

    void DoRecv();
    void DoSend(const char* packet);
};
