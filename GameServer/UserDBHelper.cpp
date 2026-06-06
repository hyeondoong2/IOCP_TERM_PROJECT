#include "pch.h"
#include "UserDBHelper.h"
#include <iostream>

namespace UserDBHelper
{
    bool IsUserRegistered(DBConnection& db, const std::wstring& name)
    {
        std::wstring query = L"SELECT CharacterID FROM dbo.Characters WHERE Name = N'" + name + L"'";
        if (db.Execute(query) && db.Fetch())
            return true;
        return false;
    }

    bool AddUserInfoInDataBase(DBConnection& db, const std::wstring& name, short x, short y)
    {
        std::wstring query =
            L"INSERT INTO dbo.Characters "
            L"(Name, Level, Exp, HP, MaxHP, PosX, PosY) "
            L"VALUES (N'" + name + L"', 1, 0, 100, 100, "
            + std::to_wstring(x) + L", "
            + std::to_wstring(y) + L")";

        return db.Execute(query);
    }

    DB_USER_INFO ExtractUserInfo(DBConnection& db, const std::wstring& name)
    {
        DB_USER_INFO info{};
        std::wstring query = L"SELECT PosX, PosY, Level, Exp FROM dbo.Characters WHERE Name = N'" + name + L"'";

        if (db.Execute(query) && db.Fetch())
        {
            info.x = static_cast<short>(db.GetInt(1));
            info.y = static_cast<short>(db.GetInt(2));
            info.level = static_cast<uint8_t>(db.GetInt(3));
            info.exp = static_cast<uint32_t>(db.GetInt(4));
        }
        return info;
    }

    bool SaveUserInfo(DBConnection& db, const std::wstring& name, short x, short y, uint8_t level, uint32_t exp)
    {
        std::wstring query = L"UPDATE dbo.Characters SET PosX = " + std::to_wstring(x)
            + L", PosY = " + std::to_wstring(y)
            + L", Level = " + std::to_wstring(level)
            + L", Exp = " + std::to_wstring(exp)
            + L" WHERE Name = N'" + name + L"'";

        return db.Execute(query);
    }
}