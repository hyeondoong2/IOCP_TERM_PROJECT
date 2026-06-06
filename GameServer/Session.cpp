#include "pch.h"
#include "Session.h"
#include "SessionManager.h"
#include "GameLogicThread.h"
#include "DBThread.h"
#include "UserDBHelper.h"
#include "Player.h"

Session::Session(SOCKET socket)
    : _socket(socket), _st(ST_ALLOC)
{
}

Session::~Session()
{
    if (_socket != INVALID_SOCKET)
    {
        ::closesocket(_socket);
        _socket = INVALID_SOCKET;
    }
}

void Session::SetId(int id)
{
    _id = id;
}

int Session::GetId() const
{
    return _id;
}

void Session::Disconnect()
{
    SOCKET closeSocket = INVALID_SOCKET;

    {
        std::lock_guard<std::mutex> lock(_mutex);

        if (_socket == INVALID_SOCKET)
            return;

        closeSocket = _socket;
        _socket = INVALID_SOCKET;
        _st = ST_FREE;
    }

    ::closesocket(closeSocket);

    if (_id != -1 && GSessionManager != nullptr)
        GSessionManager->Remove(_id);
}

void Session::DoRecv()
{
    if (_socket == INVALID_SOCKET)
        return;

    DWORD recvBytes = 0;
    DWORD recvFlag = 0;

    std::shared_ptr<Session> self = shared_from_this();
    _recvOverlapped.ReadyToRecv(self, _prevRemain);

    const int ret = ::WSARecv(
        _socket,
        &_recvOverlapped._wsabuf,
        1,
        &recvBytes,
        &recvFlag,
        &_recvOverlapped._over,
        nullptr);

    if (ret == SOCKET_ERROR)
    {
        const int err = ::WSAGetLastError();
        if (err != WSA_IO_PENDING)
        {
            std::cout << "WSARecv Error: " << err << "\n";
            Disconnect();
        }
    }
}

void Session::DoSend(const char* packet)
{
    if (packet == nullptr || _socket == INVALID_SOCKET)
        return;

    const int packetSize = static_cast<unsigned char>(packet[0]);
    if (packetSize <= 0 || packetSize > BUF_SIZE)
        return;

    SendOverlapped* sendOver = new SendOverlapped(shared_from_this(), packet);

    bool needPost = false;
    {
        std::lock_guard<std::mutex> lock(_sendMutex);
        needPost = _sendQueue.empty();
        _sendQueue.push(sendOver);
    }

    // 이미 전송 중인 패킷이 있다면 OnSend에서 순서대로 다음 패킷을 걸어준다.
    if (needPost)
        PostSend(sendOver);
}

void Session::OnRecv(DWORD bytes)
{
    // 클라이언트가 연결을 끊은 경우
    if (bytes == 0)
    {
        Disconnect();
        return;
    }

    int remainData = static_cast<int>(bytes) + _prevRemain;
    char* packetStart = _recvOverlapped._recv_buf;

    while (remainData > 0)
    {
        const int packetSize = static_cast<unsigned char>(packetStart[0]);

        if (packetSize <= 0 || packetSize > BUF_SIZE)
        {
            std::cout << "Invalid Packet Size. SessionId: " << _id << "\n";
            Disconnect();
            return;
        }

        if (packetSize > remainData)
            break;

        ProcessPacket(packetStart);

        packetStart += packetSize;
        remainData -= packetSize;
    }

    _prevRemain = remainData;

    if (_prevRemain > 0)
        memmove(_recvOverlapped._recv_buf, packetStart, _prevRemain);

    if (_prevRemain >= BUF_SIZE)
    {
        std::cout << "Recv Buffer Full. SessionId: " << _id << "\n";
        Disconnect();
        return;
    }

    DoRecv();
}

void Session::OnSend(SendOverlapped* sendOver)
{
    if (sendOver == nullptr)
        return;

    SendOverlapped* nextSend = nullptr;
    std::queue<SendOverlapped*> clearQueue;

    {
        std::lock_guard<std::mutex> lock(_sendMutex);

        if (!_sendQueue.empty() && _sendQueue.front() == sendOver)
            _sendQueue.pop();

        if (_socket == INVALID_SOCKET)
        {
            std::swap(clearQueue, _sendQueue);
        }
        else if (!_sendQueue.empty())
        {
            nextSend = _sendQueue.front();
        }
    }

    delete sendOver;

    while (!clearQueue.empty())
    {
        delete clearQueue.front();
        clearQueue.pop();
    }

    if (nextSend != nullptr)
        PostSend(nextSend);
}

void Session::send_login_fail_packet()
{
    S2C_LoginResult p;
    p.size = sizeof(S2C_LoginResult);
    p.type = S2C_LOGIN_RESULT;
    p.success = false;
    std::string message = "Login Fail";
    strcpy_s(p.message, message.c_str());
    DoSend(reinterpret_cast<const char*>(&p));
}

void Session::send_login_success_packet()
{
    S2C_LoginResult p;
    p.size = sizeof(S2C_LoginResult);
    p.type = S2C_LOGIN_RESULT;
    p.success = true;
    std::string message = "Login Successful";
    strcpy_s(p.message, message.c_str());
    DoSend(reinterpret_cast<const char*>(&p));
}

void Session::send_avatar_info_packet()
{
    std::shared_ptr<Player> owner = _owner.lock();

    if (!owner)
    {
        std::cout << "send_avatar_info_packet 실패: owner가 만료됨\n";
        return;
    }

    S2C_AvatarInfo p{};
    p.size = sizeof(S2C_AvatarInfo);
    p.type = S2C_AVATAR_INFO;

    p.playerId = owner->_id;
    p.visualId = owner->_visualId;

    p.x = owner->_x;
    p.y = owner->_y;

    p.hp = owner->_hp;
    p.max_hp = owner->_maxHp;
    p.exp = owner->_exp;
    p.level = owner->_level;

    DoSend(reinterpret_cast<const char*>(&p));
}

bool Session::PostSend(SendOverlapped* sendOver)
{
    DWORD sendBytes = 0;

    const int ret = ::WSASend(
        _socket,
        &sendOver->_wsabuf,
        1,
        &sendBytes,
        0,
        &sendOver->_over,
        nullptr);

    if (ret == SOCKET_ERROR)
    {
        const int err = ::WSAGetLastError();
        if (err != WSA_IO_PENDING)
        {
            std::cout << "WSASend Error: " << err << "\n";

            std::queue<SendOverlapped*> clearQueue;
            {
                std::lock_guard<std::mutex> lock(_sendMutex);
                if (!_sendQueue.empty() && _sendQueue.front() == sendOver)
                    _sendQueue.pop();

                std::swap(clearQueue, _sendQueue);
            }

            delete sendOver;

            while (!clearQueue.empty())
            {
                delete clearQueue.front();
                clearQueue.pop();
            }

            Disconnect();
            return false;
        }
    }

    return true;
}

void Session::ProcessPacket(char* packet)
{
    if (packet == nullptr)
        return;

    PACKET_TYPE type{};
    memcpy(&type, packet + sizeof(unsigned char), sizeof(PACKET_TYPE));

    switch (type)
    {
    case PACKET_TYPE::C2S_LOGIN:
    {
        C2S_Login* loginPacket = reinterpret_cast<C2S_Login*>(packet);
        std::string username(loginPacket->username);

        std::wstring wUsername(username.begin(), username.end());

        // logic -> db -> logic
        GGameLogicThread->PostEvent([self = shared_from_this(), username, wUsername]()
            {
                GDBManager->PostTask([self, username, wUsername](DBConnection& db)
                    {
                        short dbX = 0;
                        short dbY = 0;
                        bool isSuccess = false;

                        if (UserDBHelper::IsUserRegistered(db, wUsername))
                        {
                            DB_USER_INFO info = UserDBHelper::ExtractUserInfo(db, wUsername);
                            dbX = info.x;
                            dbY = info.y;
                            isSuccess = true;
                        }
                        else
                        {
                            if (UserDBHelper::AddUserInfoInDataBase(db, wUsername, 0, 0))
                            {
                                dbX = rand() % WORLD_WIDTH;
                                dbY = rand() % WORLD_HEIGHT;
                                isSuccess = true;
                            }
                            else
                            {
                                std::cout << "INSERT 실패\n";
                            }
                        }

                        GGameLogicThread->PostEvent([self, username, dbX, dbY, isSuccess]()
                            {
                                if (!isSuccess)
                                {
                                    self->send_login_fail_packet();
                                    std::cout << username << " 로그인 실패 (DB 에러)\n";
                                    return;
                                }

                                std::shared_ptr<Player> newPlayer = std::make_shared<Player>();

                                newPlayer->InitFromLogin(self->GetId(), username, dbX, dbY, self);
                                self->_owner = newPlayer;  

                                self->send_login_success_packet();

                                std::cout << username << " 로그인 성공! 좌표: (" << dbX << ", " << dbY << ")\n";

                                self->send_avatar_info_packet();

                                // 여기서 주변 유저 찾아서 AddPlayer 패킷 쏘기!
                            });
                    });
            });

        break;
    }
    case PACKET_TYPE::C2S_MOVE:
        break;
    case PACKET_TYPE::C2S_CHAT:
        break;
    case PACKET_TYPE::C2S_ATTACK:
        break;
    case PACKET_TYPE::C2S_TELEPORT:
        break;
    case PACKET_TYPE::C2S_LOGOUT:
        break;
    default:
        std::cout << "Unknown Packet Type. SessionId: " << _id
            << ", Type: " << static_cast<int>(type) << "\n";
    }
}

