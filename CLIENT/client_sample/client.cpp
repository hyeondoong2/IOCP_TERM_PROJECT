#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <iostream>
#include <unordered_map>
#include <Windows.h>
#include <chrono>
#include <fstream>
#include <sstream>
using namespace std;

#include "..\..\GameServer\GameServer\protocol_2026.h"

sf::TcpSocket s_socket;

constexpr auto SCREEN_WIDTH = 16;
constexpr auto SCREEN_HEIGHT = 16;

constexpr auto TILE_WIDTH = 64;
constexpr auto WINDOW_WIDTH = SCREEN_WIDTH * TILE_WIDTH;   // size of window
constexpr auto WINDOW_HEIGHT = SCREEN_WIDTH * TILE_WIDTH;

int g_bgMapData1[WORLD_HEIGHT][WORLD_WIDTH]; // 2000x2000 배경 타일 번호 저장
int g_bgMapData2[WORLD_HEIGHT][WORLD_WIDTH]; // 2000x2000 배경 타일 번호 저장
int g_bgObstacleData1[WORLD_HEIGHT][WORLD_WIDTH];
int g_bgObstacleData2[WORLD_HEIGHT][WORLD_WIDTH];

sf::Texture* g_loginTexture;
sf::Sprite* g_loginSprite;

sf::Texture* g_uiStatTexture;
sf::Sprite* g_uiStatSprite;

sf::Text g_uiNameText;
sf::Text g_uiLvText;
sf::Text g_uiExpText;
sf::Text g_uiHpText;

sf::Texture* g_tilesetTexture;
sf::Sprite* g_tileSprite;

sf::Texture* g_texBunnyIdle;
sf::Texture* g_texBunnyRun;
sf::Texture* g_texBunnyAttack;
sf::Texture* g_texBunnyHurt;
sf::Texture* g_texBunnyDeath;

sf::Texture* g_BlueSlimeIdle;
sf::Texture* g_BlueSlimeAttack;
sf::Texture* g_BlueSlimeHurt;
sf::Texture* g_BlueSlimeDeath;

sf::Texture* g_RedSlimeIdle;
sf::Texture* g_RedSlimeAttack;
sf::Texture* g_RedSlimeHurt;
sf::Texture* g_RedSlimeDeath;


sf::Texture* g_CowIdle;
sf::Texture* g_ChickenIdle;

constexpr int TILESET_COLUMNS = 57;
constexpr int SPACING = 4;

int g_left_x;
int g_top_y;
int g_myid;

sf::RenderWindow* g_window;
sf::Font g_font;

class OBJECT
{
private:
    bool m_showing;
    sf::Sprite m_sprite;

    sf::Text m_name;
    sf::Text m_chat;
    chrono::system_clock::time_point m_mess_end_time;

    // --- 애니메이션용 변수 추가 ---
    bool m_isAnimated = false; // 이 객체가 애니메이션을 사용하는가?
    int m_dir = 3;             // 0:위, 1:오른쪽, 2:왼쪽, 3:앞(아래)
    int m_state = 0;           // 0:Idle, 1:Run, 2:Hurt, 3:Attack, 4:Death
    int m_currentFrame = 0;    // 현재 프레임 인덱스
    sf::Clock m_animClock;     // 스프라이트 프레임 변경용 타이머
    sf::Clock m_moveClock;     // 이동 상태(Run) 유지용 타이머
    sf::Clock m_hurtClock;
    sf::Clock m_attackClock;

public:
    int id;
    int m_x, m_y;
    char name[MAX_NAME_LEN];
    int m_hp;
    int m_max_hp;
    int m_exp;
    int m_lv;
    bool m_isDead = false;
    bool m_isDeathAnimDone = false;

    OBJECT(sf::Texture& t, int x, int y, int x2, int y2)
    {
        m_showing = false;
        m_sprite.setTexture(t);
        m_sprite.setTextureRect(sf::IntRect(x, y, x2, y2));
        set_name("NONAME");
        m_mess_end_time = chrono::system_clock::now();
    }
    OBJECT()
    {
        m_showing = false;
    }
    void show()
    {
        m_showing = true;
    }
    void hide()
    {
        m_showing = false;
    }

    void set_animated(bool val) { m_isAnimated = val; }

    void a_move(int x, int y)
    {
        m_sprite.setPosition((float)x, (float)y);
    }

    void a_draw()
    {
        g_window->draw(m_sprite);
    }

    void move(int x, int y)
    {
        if (m_isAnimated)
        {
            // 이전 위치와 비교하여 바라보는 방향 결정
            if (x > m_x) m_dir = 1;      // X가 증가하면 오른쪽
            else if (x < m_x) m_dir = 2; // X가 감소하면 왼쪽
            else if (y < m_y) m_dir = 0; // Y가 감소하면 위
            else if (y > m_y) m_dir = 3; // Y가 증가하면 앞(아래)

            m_state = 1;           // 달리기(Run) 상태로 변경
            m_moveClock.restart(); // 마지막 이동 시간 리셋
        }
        m_x = x;
        m_y = y;
    }

    void set_hurt()
    {
        if (!m_isAnimated) return;
        m_state = 2;
        m_currentFrame = 0;
        m_hurtClock.restart();
    }

    void set_attack()
    {
        if (!m_isAnimated) return;
        m_state = 3;
        m_currentFrame = 0;
        m_attackClock.restart();
    }

    void set_death()
    {
        if (!m_isAnimated) return;
        m_state = 4;
        m_currentFrame = 0;
        m_isDead = true;
        m_isDeathAnimDone = false;
    }

    bool is_death_animation_finished()
    {
        return m_isDead && m_isDeathAnimDone;
    }

    void draw()
    {
        if (false == m_showing) return;

        if (m_isAnimated)
        {
            // hurt 0.3초 후 idle 복귀
            if (m_state == 2 && m_hurtClock.getElapsedTime().asSeconds() > 0.3f)
            {
                m_state = 0;
                m_currentFrame = 0;
            }

            // attack 0.8초 후 idle 복귀 (8프레임 * 0.1초)
            if (m_state == 3 && m_attackClock.getElapsedTime().asSeconds() > 0.8f)
            {
                m_state = 0;
                m_currentFrame = 0;
            }

            if (m_state == 1 && m_moveClock.getElapsedTime().asSeconds() > 0.15f)
            {
                m_state = 0;
                m_currentFrame = 0;
            }

            if (m_animClock.getElapsedTime().asSeconds() > 0.1f)
            {
                m_currentFrame++;
                m_animClock.restart();
            }

            sf::Texture* targetTexture = nullptr;
            int maxFrames = 1;

            if (id < MAX_PLAYERS)
            {
                switch (m_state)
                {
                case 0: targetTexture = g_texBunnyIdle;   maxFrames = 5; break;
                case 1: targetTexture = g_texBunnyRun;    maxFrames = 8; break;
                case 2: targetTexture = g_texBunnyHurt;   maxFrames = 2; break;
                case 3: targetTexture = g_texBunnyAttack; maxFrames = 8; break;
                case 4: targetTexture = g_texBunnyDeath; maxFrames = 12; break;
                }
            }
            else
            {
                if (id >= RED_SLIME_ID_START)
                {
                    switch (m_state)
                    {
                    case 2: targetTexture = g_RedSlimeHurt;   maxFrames = 2; break;
                    case 3: targetTexture = g_RedSlimeAttack; maxFrames = 8; break;
                    case 4: targetTexture = g_RedSlimeDeath; maxFrames = 4; break;
                    default:targetTexture = g_RedSlimeIdle;   maxFrames = 8; break;
                    }
                }
                else if (id >= COW_ID_START)
                {
                    m_state = (m_state >= 2) ? 0 : m_state;
                    targetTexture = g_CowIdle;
                    maxFrames = 8;
                }
                else if (id >= CHICKEN_ID_START)
                {
                    m_state = (m_state >= 2) ? 0 : m_state;
                    targetTexture = g_ChickenIdle;
                    maxFrames = 8;
                }
                else  // 블루슬라임
                {
                    switch (m_state)
                    {
                    case 2: targetTexture = g_BlueSlimeHurt;   maxFrames = 2; break;
                    case 3: targetTexture = g_BlueSlimeAttack; maxFrames = 8; break;
                    case 4: targetTexture = g_BlueSlimeDeath; maxFrames = 4; break;
                    default:targetTexture = g_BlueSlimeIdle;   maxFrames = 8; break;
                    }
                }
            }

            if (m_state == 4)
            {
                if (m_currentFrame >= maxFrames)
                {
                    m_currentFrame = maxFrames - 1;
                    m_isDeathAnimDone = true;
                }
            }
            else
            {
                m_currentFrame %= maxFrames;
            }

            if (targetTexture) m_sprite.setTexture(*targetTexture);
            m_sprite.setTextureRect(sf::IntRect(m_currentFrame * 64, m_dir * 64, 64, 64));
        }

        float scale = 3.0f;
        m_sprite.setScale(scale, scale);

        m_sprite.setOrigin(32.f, 32.f);

        float rx = (m_x - g_left_x) * TILE_WIDTH + (TILE_WIDTH / 2.f);
        float ry = (m_y - g_top_y) * TILE_WIDTH + (TILE_WIDTH / 2.f);

        m_sprite.setPosition(rx, ry);
        g_window->draw(m_sprite);

        auto size = m_name.getGlobalBounds();
        float spriteCenterX = rx;

        if (m_mess_end_time < chrono::system_clock::now())
        {
            if (id == g_myid)
            {
                m_name.setPosition(spriteCenterX - size.width / 2.f, ry - (32.f * scale) - 5.f);
                g_window->draw(m_name);
            }
        }
        else
        {
            m_chat.setPosition(spriteCenterX - size.width / 2.f, ry - (32.f * scale) - 15.f);
            g_window->draw(m_chat);
        }
    }
    void set_name(const char str[])
    {
        strcpy_s(name, MAX_NAME_LEN, str);

        m_name.setFont(g_font);
        m_name.setString(str);
        m_name.setCharacterSize(25);
        if (id < MAX_PLAYERS) m_name.setFillColor(sf::Color(255, 255, 255));
        else m_name.setFillColor(sf::Color(255, 255, 0));
        m_name.setStyle(sf::Text::Bold);
    }

    void set_chat(const char str[])
    {
        m_chat.setFont(g_font);
        m_chat.setString(str);
        m_name.setCharacterSize(20);
        m_chat.setFillColor(sf::Color(255, 255, 255));
        m_chat.setStyle(sf::Text::Bold);
        m_mess_end_time = chrono::system_clock::now() + chrono::seconds(3);
    }
};


OBJECT avatar;
unordered_map <int, OBJECT> players;

OBJECT white_tile;
OBJECT black_tile;

sf::Texture* board;
sf::Texture* pieces;

// 장애물 판단
bool IsBlocked(int x, int y)
{
    if (x < 0 || x >= WORLD_WIDTH || y < 0 || y >= WORLD_HEIGHT) return true;
    int obs1 = g_bgObstacleData1[y][x];
    int obs2 = g_bgObstacleData2[y][x];
    std::cout << "IsBlocked(" << x << ", " << y << ") obs1=" << obs1 << " obs2=" << obs2 << std::endl;
    return obs1 != -1 || obs2 != -1;
}

// --- 배경 CSV 읽기 함수 
void LoadMapLayer1(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "배경 CSV 파일 로드 실패: " << filename << std::endl;
        return;
    }

    std::string line;
    int y = 0;
    while (std::getline(file, line) && y < WORLD_HEIGHT)
    {
        std::stringstream ss(line);
        std::string cell;
        int x = 0;
        while (std::getline(ss, cell, ',') && x < WORLD_WIDTH)
        {
            g_bgMapData1[y][x] = std::stoi(cell); // 타일 ID 그대로 저장
            x++;
        }
        y++;
    }
    file.close();
    std::cout << "배경 맵 데이터 로드 완료!" << std::endl;
}

void LoadMapLayer2(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "배경 CSV 파일 로드 실패: " << filename << std::endl;
        return;
    }

    std::string line;
    int y = 0;
    while (std::getline(file, line) && y < WORLD_HEIGHT)
    {
        std::stringstream ss(line);
        std::string cell;
        int x = 0;
        while (std::getline(ss, cell, ',') && x < WORLD_WIDTH)
        {
            g_bgMapData2[y][x] = std::stoi(cell); // 타일 ID 그대로 저장
            x++;
        }
        y++;
    }
    file.close();
    std::cout << "배경 맵 데이터 로드 완료!" << std::endl;
}

// --- 장애물 CSV 읽기 함수 
void LoadObstacleLayer1(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "장애물 CSV 파일 로드 실패: " << filename << std::endl;
        return;
    }

    std::string line;
    int y = 0;
    while (std::getline(file, line) && y < WORLD_HEIGHT)
    {
        std::stringstream ss(line);
        std::string cell;
        int x = 0;
        while (std::getline(ss, cell, ',') && x < WORLD_WIDTH)
        {
            g_bgObstacleData1[y][x] = std::stoi(cell); // 타일 ID 그대로 저장
            x++;
        }
        y++;
    }
    file.close();
    std::cout << "장애물 데이터 로드 완료!" << std::endl;
}

// --- 장애물 CSV 읽기 함수 
void LoadObstacleLayer2(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "장애물 CSV 파일 로드 실패: " << filename << std::endl;
        return;
    }

    std::string line;
    int y = 0;
    while (std::getline(file, line) && y < WORLD_HEIGHT)
    {
        std::stringstream ss(line);
        std::string cell;
        int x = 0;
        while (std::getline(ss, cell, ',') && x < WORLD_WIDTH)
        {
            g_bgObstacleData2[y][x] = std::stoi(cell); // 타일 ID 그대로 저장
            x++;
        }
        y++;
    }
    file.close();
    std::cout << "장애물 데이터 로드 완료!" << std::endl;
}

void DrawStatUI()
{
    g_window->draw(*g_uiStatSprite);
    g_window->draw(g_uiNameText);
    g_window->draw(g_uiLvText);
    g_window->draw(g_uiExpText);
    g_window->draw(g_uiHpText);
}

void UpdateStatUI(const std::string& name, int lv, int exp, int hp, int max_hp)
{
    g_uiNameText.setString(name);
    g_uiLvText.setString(std::to_string(lv));
    g_uiExpText.setString(std::to_string(exp));
    g_uiHpText.setString(std::to_string(hp) + " / " + std::to_string(max_hp));
}


void client_initialize()
{
    pieces = new sf::Texture;
    pieces->loadFromFile("chess2.png");
    pieces->setSmooth(false);

    // stat ui
    g_uiStatTexture = new sf::Texture;
    g_uiStatTexture->loadFromFile("stat.png");
    g_uiStatTexture->setSmooth(false);

    g_uiStatSprite = new sf::Sprite(*g_uiStatTexture);

    float uiWidth = g_uiStatTexture->getSize().x;
    float startX = 20.f;
    float startY = 20.f;
    g_uiStatSprite->setPosition(startX, startY);

    auto setupText = [](sf::Text& t, int size)
        {
            t.setFont(g_font);
            t.setCharacterSize(size);
            t.setFillColor(sf::Color(103, 48, 36));
            t.setOutlineThickness(0.f);
        };

    setupText(g_uiNameText, 16);
    setupText(g_uiLvText, 16);
    setupText(g_uiExpText, 16);
    setupText(g_uiHpText, 16);

    g_uiNameText.setPosition(startX + (170.f), startY + (18.f));
    g_uiLvText.setPosition(startX + (330.f), startY + (18.f));
    g_uiExpText.setPosition(startX + (180.f), startY + (46.f));
    g_uiHpText.setPosition(startX + (330.f), startY + (46.f));

    // login
    g_loginTexture = new sf::Texture;
    g_loginTexture->loadFromFile("login.png");
    g_loginTexture->setSmooth(false);

    g_loginSprite = new sf::Sprite(*g_loginTexture);

    // player -------------------------------------------------------------
    g_texBunnyIdle = new sf::Texture;
    g_texBunnyIdle->loadFromFile("Bunny_Idle.png");
    g_texBunnyIdle->setSmooth(false);

    g_texBunnyRun = new sf::Texture;
    g_texBunnyRun->loadFromFile("Bunny_Run.png");
    g_texBunnyRun->setSmooth(false);

    g_texBunnyAttack = new sf::Texture;
    g_texBunnyAttack->loadFromFile("Bunny_Sword.png");
    g_texBunnyAttack->setSmooth(false);

    g_texBunnyHurt = new sf::Texture;
    g_texBunnyHurt->loadFromFile("Bunny_Hurt.png");
    g_texBunnyHurt->setSmooth(false);

    g_texBunnyDeath = new sf::Texture;
    g_texBunnyDeath->loadFromFile("Bunny_Death.png");
    g_texBunnyDeath->setSmooth(false);

    // blue slime -------------------------------------------------------------
    g_BlueSlimeIdle = new sf::Texture;
    g_BlueSlimeIdle->loadFromFile("Blue_Slime_Sprites/Slime_Idle.png");
    g_BlueSlimeIdle->setSmooth(false);

    g_BlueSlimeAttack = new sf::Texture;
    g_BlueSlimeAttack->loadFromFile("Blue_Slime_Sprites/Slime_Attack.png");
    g_BlueSlimeAttack->setSmooth(false);

    g_BlueSlimeHurt = new sf::Texture;
    g_BlueSlimeHurt->loadFromFile("Blue_Slime_Sprites/Slime_Hurt.png");
    g_BlueSlimeHurt->setSmooth(false);

    g_BlueSlimeDeath = new sf::Texture;
    g_BlueSlimeDeath->loadFromFile("Blue_Slime_Sprites/Slime_Death.png");
    g_BlueSlimeDeath->setSmooth(false);

    // red slime -------------------------------------------------------------
    g_RedSlimeIdle = new sf::Texture;
    g_RedSlimeIdle->loadFromFile("Red_Slime_Sprites/Red_Slime_Idle.png");
    g_RedSlimeIdle->setSmooth(false);

    g_RedSlimeAttack = new sf::Texture;
    g_RedSlimeAttack->loadFromFile("Red_Slime_Sprites/Red_Slime_Attack.png");
    g_RedSlimeAttack->setSmooth(false);

    g_RedSlimeHurt = new sf::Texture;
    g_RedSlimeHurt->loadFromFile("Red_Slime_Sprites/Red_Slime_Hurt.png");
    g_RedSlimeHurt->setSmooth(false);

    g_RedSlimeDeath = new sf::Texture;
    g_RedSlimeDeath->loadFromFile("Red_Slime_Sprites/Red_Slime_Death.png");
    g_RedSlimeDeath->setSmooth(false);

    g_CowIdle = new sf::Texture;
    g_CowIdle->loadFromFile("COW/Cow_Idle.png");
    g_CowIdle->setSmooth(false);

    g_ChickenIdle = new sf::Texture;
    g_ChickenIdle->loadFromFile("CHICKEN/Chicken_Idle.png");
    g_ChickenIdle->setSmooth(false);

    memset(g_bgObstacleData1, -1, sizeof(g_bgObstacleData1));
    memset(g_bgObstacleData2, -1, sizeof(g_bgObstacleData2));


    if (false == g_font.loadFromFile("온글잎 박다현체.ttf"))
    {
        cout << "Font Loading Error!\n";
        exit(-1);
    }

    g_tilesetTexture = new sf::Texture;
    g_tileSprite = new sf::Sprite;

    if (!g_tilesetTexture->loadFromFile("roguelikeSheet_transparent.png"))
    {
    }
    g_tilesetTexture->setSmooth(false);
    g_tileSprite->setTexture(*g_tilesetTexture);

    LoadMapLayer1("map_ground1.csv");
    LoadMapLayer2("map_ground2.csv");
    LoadObstacleLayer1("map_obstacle.csv");
    LoadObstacleLayer2("map_obstacle2.csv");

    avatar = OBJECT{ *g_texBunnyIdle, 0, 192, 64, 64 }; 
    avatar.set_animated(true); 
    avatar.move(4, 4);
}

void client_finish()
{
    players.clear();
    delete board;
    delete pieces;

    delete g_tilesetTexture;
    delete g_tileSprite;

    delete g_texBunnyIdle;
    delete g_texBunnyRun;

    delete g_BlueSlimeAttack;
    delete g_RedSlimeAttack;
}

void ProcessPacket(char* ptr)
{
    static bool first_time = true;
    switch (ptr[1])
    {
    case S2C_LOGIN_RESULT: break;
    case S2C_AVATAR_INFO:
    {
        S2C_AvatarInfo* packet = reinterpret_cast<S2C_AvatarInfo*>(ptr);
        g_myid = packet->playerId;
        avatar.id = g_myid;
        avatar.move(packet->x, packet->y);
        avatar.m_hp = packet->hp;
        avatar.m_max_hp = packet->max_hp;
        avatar.m_exp = packet->exp;
        avatar.m_lv = packet->level;
        g_left_x = packet->x - SCREEN_WIDTH / 2;
        g_top_y = packet->y - SCREEN_HEIGHT / 2;

        UpdateStatUI(avatar.name, avatar.m_lv, avatar.m_exp, avatar.m_hp, avatar.m_max_hp);
        avatar.show();
    break;
    }

    case S2C_ADD_OBJECT:
    {
        S2C_AddObject* my_packet = reinterpret_cast<S2C_AddObject*>(ptr);
        int id = my_packet->object_id;

        // 나자신
        if (id == g_myid)
        {
            avatar.move(my_packet->x, my_packet->y);
            g_left_x = my_packet->x - SCREEN_WIDTH / 2;
            g_top_y = my_packet->y - SCREEN_HEIGHT / 2;
            avatar.show();
        }
        // 플레이어
        else if (id < MAX_PLAYERS)
        {
            players[id] = OBJECT{ *g_texBunnyIdle, 0, 192, 64, 64 };
            players[id].set_animated(true);

            players[id].id = id;
            players[id].move(my_packet->x, my_packet->y);
            players[id].set_name(my_packet->obj_name);
            players[id].show();
        }
        else
        {
            sf::Texture* targetTexture = nullptr;

            if (id >= RED_SLIME_ID_START)
            {
                targetTexture = g_RedSlimeIdle;
            }
            else if (id >= COW_ID_START)
            {
                targetTexture = g_CowIdle;
            }
            else if (id >= CHICKEN_ID_START)
            {
                targetTexture = g_ChickenIdle;
            }
            else if (id >= BLUE_SLIME_ID_START)
            {
                targetTexture = g_BlueSlimeIdle;
            }

            if (targetTexture != nullptr)
            {
                players[id] = OBJECT{ *targetTexture, 0, 0, 64, 64 };
                players[id].set_animated(true);

                players[id].id = id;
                players[id].move(my_packet->x, my_packet->y);
                players[id].set_name(my_packet->obj_name);
                players[id].show();
            }
            else
            {
                cout << "알 수 없는 NPC ID 스폰 요청: " << id << "\n";
            }
        }
        break;
    }
    case S2C_MOVE_OBJECT:
    {
        S2C_MoveObject* my_packet = reinterpret_cast<S2C_MoveObject*>(ptr);
        int other_id = my_packet->object_id;
        if (other_id == g_myid)
        {
            avatar.move(my_packet->x, my_packet->y);
            g_left_x = my_packet->x - SCREEN_WIDTH / 2;
            g_top_y = my_packet->y - SCREEN_HEIGHT / 2;
        }
        else
        {
            players[other_id].move(my_packet->x, my_packet->y);
        }
        break;
    }

    case S2C_REMOVE_OBJECT:
    {
        S2C_RemoveObject* my_packet = reinterpret_cast<S2C_RemoveObject*>(ptr);
        int other_id = my_packet->object_id;
        if (other_id == g_myid)
        {
            avatar.hide();
        }
        else
        {
            players.erase(other_id);
        }
        break;
    }
    case S2C_CHAT_MESSAGE:
    {
        S2C_ChatMessage* my_packet = reinterpret_cast<S2C_ChatMessage*>(ptr);
        int other_id = my_packet->object_id;
        if (other_id == g_myid)
        {
            avatar.set_chat(my_packet->message);
        }
        else
        {
            players[other_id].set_chat(my_packet->message);
        }

        break;
    }
    case S2C_HIT_OBJECT:
    {
        S2C_HitObject* my_packet = reinterpret_cast<S2C_HitObject*>(ptr);
        int other_id = my_packet->object_id;
        if (other_id == g_myid)
            avatar.set_hurt();
        else if (players.count(other_id))
            players[other_id].set_hurt();
        break;
    }
    case S2C_ATTACK_OBJECT:
    {
        S2C_AttackObject* my_packet = reinterpret_cast<S2C_AttackObject*>(ptr);
        int other_id = my_packet->object_id;
        if (other_id == g_myid)
            avatar.set_attack();
        else if (players.count(other_id))
            players[other_id].set_attack();
        break;
    }
    case S2C_DIE_OBJECT:
    {
        S2C_DieObject* my_packet = reinterpret_cast<S2C_DieObject*>(ptr);
        int other_id = my_packet->object_id;
        if (other_id == g_myid)
            avatar.set_death();
        else if (players.count(other_id))
            players[other_id].set_death();
        break;
    }
    case S2C_STATUS_CHANGE:
    {
        S2C_StatusChange* my_packet = reinterpret_cast<S2C_StatusChange*>(ptr);
        int other_id = my_packet->object_id;
        if (other_id == g_myid)
        {
            if (avatar.m_isDead) avatar.m_isDead = false;
            avatar.m_exp = my_packet->exp;
            avatar.m_hp = my_packet->hp;
            avatar.m_lv = my_packet->level;
            avatar.m_max_hp = my_packet->max_hp;

            UpdateStatUI(avatar.name, avatar.m_lv, avatar.m_exp, avatar.m_hp, avatar.m_max_hp);
        }

    }
    default:
        printf("Unknown PACKET type [%d]\n", ptr[1]);
    }
}

void process_data(char* net_buf, size_t io_byte)
{
    char* ptr = net_buf;
    static size_t in_packet_size = 0;
    static size_t saved_packet_size = 0;
    static char packet_buffer[BUF_SIZE];

    while (0 != io_byte)
    {
        if (0 == in_packet_size) in_packet_size = ptr[0];
        if (io_byte + saved_packet_size >= in_packet_size)
        {
            memcpy(packet_buffer + saved_packet_size, ptr, in_packet_size - saved_packet_size);
            ProcessPacket(packet_buffer);
            ptr += in_packet_size - saved_packet_size;
            io_byte -= in_packet_size - saved_packet_size;
            in_packet_size = 0;
            saved_packet_size = 0;
        }
        else
        {
            memcpy(packet_buffer + saved_packet_size, ptr, io_byte);
            saved_packet_size += io_byte;
            io_byte = 0;
        }
    }
}

void client_main()
{
    char net_buf[BUF_SIZE];
    size_t	received;

    auto recv_result = s_socket.receive(net_buf, BUF_SIZE, received);
    if (recv_result == sf::Socket::Error)
    {
        wcout << L"Recv 에러!";
        exit(-1);
    }
    if (recv_result == sf::Socket::Disconnected)
    {
        wcout << L"Disconnected\n";
        exit(-1);
    }
    if (recv_result != sf::Socket::NotReady)
        if (received > 0) process_data(net_buf, received);


    for (int i = 0; i < SCREEN_WIDTH; ++i)
    {
        for (int j = 0; j < SCREEN_HEIGHT; ++j)
        {
            int tile_x = i + g_left_x;
            int tile_y = j + g_top_y;

            if (tile_x < 0 || tile_x >= WORLD_WIDTH || tile_y < 0 || tile_y >= WORLD_HEIGHT) continue;

            int bg1_tile_id = g_bgMapData1[tile_y][tile_x];
            int bg2_tile_id = g_bgMapData2[tile_y][tile_x];
            int obs1_tile_id = g_bgObstacleData1[tile_y][tile_x];
            int obs2_tile_id = g_bgObstacleData2[tile_y][tile_x];

            if (bg1_tile_id > 0)
            {
                int actual_id1 = bg1_tile_id;

                int src_x1 = (actual_id1 % TILESET_COLUMNS) * (TILE_WIDTH + SPACING);
                int src_y1 = (actual_id1 / TILESET_COLUMNS) * (TILE_WIDTH + SPACING);

                g_tileSprite->setTextureRect(sf::IntRect(src_x1, src_y1, TILE_WIDTH, TILE_WIDTH));
                g_tileSprite->setScale(1.05f, 1.05f);
                g_tileSprite->setPosition(i * TILE_WIDTH, j * TILE_WIDTH);
                g_window->draw(*g_tileSprite);
            }

            if (bg2_tile_id > 0)
            {
                int actual_id1 = bg2_tile_id;

                int src_x1 = (actual_id1 % TILESET_COLUMNS) * (TILE_WIDTH + SPACING);
                int src_y1 = (actual_id1 / TILESET_COLUMNS) * (TILE_WIDTH + SPACING);

                g_tileSprite->setTextureRect(sf::IntRect(src_x1, src_y1, TILE_WIDTH, TILE_WIDTH));
                g_tileSprite->setScale(1.05f, 1.05f);
                g_tileSprite->setPosition(i * TILE_WIDTH, j * TILE_WIDTH);
                g_window->draw(*g_tileSprite);
            }


            if (obs1_tile_id > 0)
            {
                int actual_id2 = obs1_tile_id;

                int src_x2 = (actual_id2 % TILESET_COLUMNS) * (TILE_WIDTH + SPACING);
                int src_y2 = (actual_id2 / TILESET_COLUMNS) * (TILE_WIDTH + SPACING);

                g_tileSprite->setTextureRect(sf::IntRect(src_x2, src_y2, TILE_WIDTH, TILE_WIDTH));
                g_tileSprite->setScale(1.05f, 1.05f);
                g_tileSprite->setPosition(i * TILE_WIDTH, j * TILE_WIDTH);
                g_window->draw(*g_tileSprite);
            }
            if (obs2_tile_id > 0)
            {
                int actual_id2 = obs2_tile_id;

                int src_x2 = (actual_id2 % TILESET_COLUMNS) * (TILE_WIDTH + SPACING);
                int src_y2 = (actual_id2 / TILESET_COLUMNS) * (TILE_WIDTH + SPACING);

                g_tileSprite->setTextureRect(sf::IntRect(src_x2, src_y2, TILE_WIDTH, TILE_WIDTH));
                g_tileSprite->setScale(1.05f, 1.05f);
                g_tileSprite->setPosition(i * TILE_WIDTH, j * TILE_WIDTH);
                g_window->draw(*g_tileSprite);
            }
        }
    }

    if (avatar.m_isDead && avatar.is_death_animation_finished())
    {
        avatar.hide(); 
    }

    avatar.draw();

    for (auto it = players.begin(); it != players.end(); )
    {
        it->second.draw();

        if (it->second.is_death_animation_finished())
        {
            it = players.erase(it); // 완전히 죽었으면 맵에서 지움
        }
        else
        {
            ++it;
        }
    }

    // position
    sf::Text text;
    text.setFont(g_font);
    char buf[100];
    sprintf_s(buf, "(%d, %d)", avatar.m_x, avatar.m_y);
    text.setString(buf);
    text.setPosition(WINDOW_WIDTH - 120.f, 20.f);

    g_window->draw(text);
}

void send_packet(void* packet)
{
    unsigned char* p = reinterpret_cast<unsigned char*>(packet);
    size_t sent = 0;
    s_socket.send(packet, p[0], sent);
}

int main()
{
    std::string ipAddress;
    std::cout << "접속할 IP 주소를 입력하세요: ";
    std::cin >> ipAddress;

    wcout.imbue(locale("korean"));

    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "2D CLIENT");
    g_window = &window;

    client_initialize();

    sf::Text ui_loginText;
    ui_loginText.setFont(g_font);
    ui_loginText.setCharacterSize(30);
    ui_loginText.setFillColor(sf::Color(103, 48, 36));

    ui_loginText.setPosition(WINDOW_WIDTH / 2.f - 50.f, WINDOW_HEIGHT / 2.f - 5);

    std::string player_name = "";
    bool isLoginScreen = true;

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();


            if (isLoginScreen)
            {

                if (event.type == sf::Event::TextEntered)
                {
                    if (event.text.unicode == '\b')
                    {
                        if (!player_name.empty())
                            player_name.pop_back();
                    }

                    else if (event.text.unicode < 128 && event.text.unicode > 31 && player_name.size() < 19)
                    {
                        player_name += static_cast<char>(event.text.unicode);
                    }
                }
                else if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Enter)
                {
                    if (!player_name.empty())
                    {
                        sf::Socket::Status status = s_socket.connect(ipAddress, PORT);
                        s_socket.setBlocking(false);

                        if (status != sf::Socket::Done)
                        {
                            wcout << L"서버와 연결할 수 없습니다.\n";
                            exit(-1);
                        }

                        C2S_Login p;
                        p.size = sizeof(p);
                        p.type = C2S_LOGIN;
                        strcpy_s(p.username, player_name.c_str());
                        send_packet(&p);

                        avatar.set_name(p.username);
                        isLoginScreen = false;
                    }
                }
            }

            else
            {
                if (event.type == sf::Event::KeyPressed)
                {
                    if (avatar.m_isDead) continue;

                    int move_x = 0;
                    int move_y = 0;
                    switch (event.key.code)
                    {
                    case sf::Keyboard::Left:  move_x = -1; break;
                    case sf::Keyboard::Right: move_x = 1;  break;
                    case sf::Keyboard::Up:    move_y = -1; break;
                    case sf::Keyboard::Down:  move_y = 1;  break;
                    case sf::Keyboard::A: {
                        C2S_Attack p;
                        p.size = sizeof(p);
                        p.type = C2S_ATTACK;
                        send_packet(&p);
                        break;
                    }
                    case sf::Keyboard::Escape: window.close(); break;
                    }

                    if (move_x != 0 || move_y != 0)
                    {
                        int next_x = avatar.m_x + move_x;
                        int next_y = avatar.m_y + move_y;

                        if (!IsBlocked(next_x, next_y))
                        {
                            C2S_Move p;
                            p.size = sizeof(p);
                            p.type = C2S_MOVE;
                            p.x = next_x;
                            p.y = next_y;
                            send_packet(&p);
                        }
                    }
                }
            }
        }

        window.clear();

        if (isLoginScreen)
        {
            if (g_loginSprite != nullptr)
                window.draw(*g_loginSprite);

            ui_loginText.setString(player_name);
            window.draw(ui_loginText);
        }
        else
        {
            client_main();
            DrawStatUI();
        }

        window.display();
    }

    client_finish();
    return 0;
}