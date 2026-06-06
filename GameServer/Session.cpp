#include "pch.h"
#include "Session.h"
#include "SessionManager.h"
#include "GameLogicThread.h"
#include "DBThread.h"
#include "UserDBHelper.h"
#include "Player.h"
#include "ObjectManager.h"
#include "SectorManager.h"


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

    Logout();

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

void Session::HandleLoginPacket(C2S_Login* packet)
{
    std::string username(packet->username);
    std::wstring wUsername(username.begin(), username.end());

    GGameLogicThread->PostEvent([self = shared_from_this(), username, wUsername]()
        {
            self->ProcessLoginDatabase(username, wUsername);
        });
}

void Session::ProcessLoginDatabase(const std::string& username, const std::wstring& wUsername)
{
    GDBManager->PostTask([self = shared_from_this(), username, wUsername](DBConnection& db)
        {
            short dbX = 0, dbY = 0;
            bool isSuccess = false;

            if (UserDBHelper::IsUserRegistered(db, wUsername))
            {
                DB_USER_INFO info = UserDBHelper::ExtractUserInfo(db, wUsername);
                dbX = info.x; dbY = info.y;
                isSuccess = true;
            }
            else
            {
                dbX = rand() % WORLD_WIDTH;
                dbY = rand() % WORLD_HEIGHT;
                isSuccess = UserDBHelper::AddUserInfoInDataBase(db, wUsername, dbX, dbY);
            }

            GGameLogicThread->PostEvent([self, username, dbX, dbY, isSuccess]()
                {
                    self->OnLoginResult(username, dbX, dbY, isSuccess);
                });
        });
}

void Session::OnLoginResult(const std::string& username, short dbX, short dbY, bool isSuccess)
{
    if (!isSuccess)
    {
        send_login_fail_packet();
        return;
    }

    std::shared_ptr<Player> newPlayer = std::make_shared<Player>();
    newPlayer->InitFromLogin(GetId(), username, dbX, dbY, shared_from_this());
    _owner = newPlayer;

    send_login_success_packet();
    send_my_avatar_info_packet();

    GObjectManager->AddObject(newPlayer);
    GSectorManager->AddObject(newPlayer);

    GSectorManager->BroadcastSpawnInfo(newPlayer);
    GSectorManager->SendNearbyObjectsToPlayer(newPlayer);

    std::cout << username << " 로그인 성공! 좌표: (" << dbX << ", " << dbY << ")\n";
}

void Session::HandleMovePacket(C2S_Move* packet)
{
    GGameLogicThread->PostEvent([self = shared_from_this(), x = packet->x, y = packet->y, move_time = packet->move_time]()
        {
            auto player = self->_owner.lock();
            if (!player) return;

            player->_x = x;
            player->_y = y;
            player->_lastMoveTime = move_time;

            GSectorManager->UpdateObjectSector(player);
            GSectorManager->BroadcastMove(player);
        });
}

void Session::Logout()
{
    auto player = _owner.lock();
    if (!player) return;

    std::string username = player->_name;
    std::wstring wUsername(username.begin(), username.end());
    short finalX = player->_x;
    short finalY = player->_y;
    uint8_t level = player->_level;
    uint32_t exp = player->_exp;

    GDBManager->PostTask([wUsername, finalX, finalY, level, exp](DBConnection& db)
        {
            if (UserDBHelper::SaveUserInfo(db, wUsername, finalX, finalY, level, exp))
            {
                std::cout << "[DB] 캐릭터 정보 저장 완료: " << (char*)wUsername.c_str() << "\n";
            }
            else
            {
                std::cerr << "[DB] 캐릭터 정보 저장 실패!\n";
            }
        });

    GSectorManager->RemoveObject(player);
    GObjectManager->RemoveObject(player->_id);
    _owner.reset();
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
    S2C_LoginResult p{};
    p.size = sizeof(S2C_LoginResult);
    p.type = S2C_LOGIN_RESULT;
    p.success = true;
    std::string message = "Login Successful";
    strcpy_s(p.message, message.c_str());
    DoSend(reinterpret_cast<const char*>(&p));
}

void Session::send_my_avatar_info_packet()
{
    auto owner = _owner.lock();
    if (owner)
    {
        send_avatar_packet(owner);
    }
}

void Session::send_avatar_packet(std::shared_ptr<Player> target_player)
{
    if (!target_player) return;

    S2C_AvatarInfo p{};
    p.size = sizeof(S2C_AvatarInfo);
    p.type = S2C_AVATAR_INFO;
    p.playerId = target_player->_id;
    p.visualId = target_player->_visualId;
    p.x = target_player->_x;
    p.y = target_player->_y;
    p.hp = target_player->_hp;
    p.max_hp = target_player->_maxHp;
    p.exp = target_player->_exp;
    p.level = target_player->_level;

    DoSend(reinterpret_cast<const char*>(&p));
}

void Session::send_object_spawn_packet(std::shared_ptr<GameObject> obj)
{
    if (!obj) return;

    S2C_AddObject p{};
    p.size = sizeof(S2C_AddObject);
    p.type = S2C_ADD_OBJECT;
    p.object_id = obj->_id;

    p.x = obj->_x;
    p.y = obj->_y;
    p.hp = obj->_hp;
    p.max_hp = obj->_maxHp;

    strncpy_s(p.obj_name, obj->_name.c_str(), MAX_NAME_LEN - 1);

    p.exp = 0;
    p.level = 0;
    p.visual_id = 100;

    auto player = std::dynamic_pointer_cast<Player>(obj);
    if (player)
    {
        p.exp = player->_exp;
        p.level = player->_level;
        p.visual_id = player->_visualId; 
    }

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
        HandleLoginPacket(loginPacket);
        break;
    }
    case PACKET_TYPE::C2S_MOVE:
    {
        C2S_Move* movePacket = reinterpret_cast<C2S_Move*>(packet);
        HandleMovePacket(movePacket);
        break;
    }
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

