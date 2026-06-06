#pragma once
#include <string>
#include "DBConnection.h" 

struct DB_USER_INFO
{
    short x;
    short y;
    uint8_t level;
    uint32_t exp;
};

namespace UserDBHelper
{
    bool IsUserRegistered(DBConnection& db, const std::wstring& name);

    bool AddUserInfoInDataBase(DBConnection& db, const std::wstring& name, short x, short y);

    DB_USER_INFO ExtractUserInfo(DBConnection& db, const std::wstring& name);

    bool SaveUserInfo(DBConnection& db, const std::wstring& name, short x, short y, uint8_t level, uint32_t exp);
}