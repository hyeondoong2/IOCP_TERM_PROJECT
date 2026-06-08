#include "pch.h"
#include "DBConnection.h"
#include <iostream>
#include "UserDBHelper.h"

namespace
{
    bool IsOdbcSuccess(SQLRETURN ret)
    {
        return ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO;
    }

    void PrintOdbcError(SQLSMALLINT handleType, SQLHANDLE handle, const wchar_t* context)
    {
        SQLWCHAR state[6] = {};
        SQLWCHAR message[SQL_MAX_MESSAGE_LENGTH] = {};
        SQLINTEGER nativeError = 0;
        SQLSMALLINT messageLength = 0;

        std::wcerr << L"[ODBC] " << context << L" failed.\n";

        for (SQLSMALLINT i = 1; ; ++i)
        {
            SQLRETURN ret = SQLGetDiagRecW(
                handleType,
                handle,
                i,
                state,
                &nativeError,
                message,
                SQL_MAX_MESSAGE_LENGTH,
                &messageLength);

            if (!IsOdbcSuccess(ret))
                break;

            std::wcerr << L"  State: " << state
                << L", NativeError: " << nativeError
                << L", Message: " << message << L"\n";
        }
    }
}

DBConnection::DBConnection(int id)
    : _id(id)
{
}

DBConnection::~DBConnection()
{
    Disconnect();
}

bool DBConnection::Connect(const std::wstring& connectionString)
{
    if (_connected)
        return true;

    SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &_hEnv);
    if (!IsOdbcSuccess(ret))
    {
        std::wcerr << L"[ODBC] SQLAllocHandle ENV failed.\n";
        return false;
    }

    ret = SQLSetEnvAttr(_hEnv, SQL_ATTR_ODBC_VERSION, reinterpret_cast<SQLPOINTER>(SQL_OV_ODBC3), 0);
    if (!IsOdbcSuccess(ret))
    {
        PrintOdbcError(SQL_HANDLE_ENV, _hEnv, L"SQLSetEnvAttr");
        Disconnect();
        return false;
    }

    ret = SQLAllocHandle(SQL_HANDLE_DBC, _hEnv, &_hDbc);
    if (!IsOdbcSuccess(ret))
    {
        PrintOdbcError(SQL_HANDLE_ENV, _hEnv, L"SQLAllocHandle DBC");
        Disconnect();
        return false;
    }

    SQLSetConnectAttrW(_hDbc, SQL_LOGIN_TIMEOUT, reinterpret_cast<SQLPOINTER>(5), 0);

    SQLWCHAR outStr[1024] = {};
    SQLSMALLINT outStrLen = 0;

    ret = SQLDriverConnectW(
        _hDbc,
        NULL,
        reinterpret_cast<SQLWCHAR*>(const_cast<wchar_t*>(connectionString.c_str())),
        SQL_NTS,
        outStr,
        sizeof(outStr) / sizeof(SQLWCHAR),
        &outStrLen,
        SQL_DRIVER_NOPROMPT);

    if (IsOdbcSuccess(ret))
    {
        _connected = true;
        _connectionString = connectionString;
        std::wcout << L"DBConnection[" << _id << L"] Connected.\n";
        return true;
    }

    PrintOdbcError(SQL_HANDLE_DBC, _hDbc, L"SQLDriverConnectW");
    Disconnect();
    return false;
}

void DBConnection::Disconnect()
{
    if (_hDbc != SQL_NULL_HDBC)
    {
        if (_connected)
            SQLDisconnect(_hDbc);

        SQLFreeHandle(SQL_HANDLE_DBC, _hDbc);
        _hDbc = SQL_NULL_HDBC;
    }

    if (_hEnv != SQL_NULL_HENV)
    {
        SQLFreeHandle(SQL_HANDLE_ENV, _hEnv);
        _hEnv = SQL_NULL_HENV;
    }

    _connected = false;
    _connectionString.clear();
}

bool DBConnection::Execute(const std::wstring& query)
{
    //std::wcout << L"DEBUG: Executing Query -> " << query << std::endl;

    if (_hStmt != SQL_NULL_HSTMT)
    {
        ::SQLFreeHandle(SQL_HANDLE_STMT, _hStmt);
        _hStmt = SQL_NULL_HSTMT;
    }

    if (::SQLAllocHandle(SQL_HANDLE_STMT, _hDbc, &_hStmt) != SQL_SUCCESS)
        return false;

    SQLRETURN ret = ::SQLExecDirectW(_hStmt, (SQLWCHAR*)query.c_str(), SQL_NTS);

    if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO)
        return true;

    SQLWCHAR sqlState[6], message[256];
    SQLINTEGER nativeError;
    SQLSMALLINT messageLen;
    ::SQLGetDiagRecW(SQL_HANDLE_STMT, _hStmt, 1, sqlState, &nativeError, message, 256, &messageLen);

    //std::wcout << L"----------------------------------------" << std::endl;
    //std::wcout << L"[DB 에러 메시지] " << message << std::endl;
    //std::wcout << L"----------------------------------------" << std::endl;

    return false;
}

bool DBConnection::Fetch()
{
    SQLRETURN ret = ::SQLFetch(_hStmt);
    if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO)
        return true;

    return false;
}

int DBConnection::GetInt(int columnIndex)
{
    int value = 0;
    SQLLEN len = 0;

    ::SQLGetData(_hStmt, columnIndex, SQL_C_LONG, &value, 0, &len);
    return value;
}

std::wstring DBConnection::GetString(int columnIndex)
{
    wchar_t buf[256] = { 0, };
    SQLLEN len = 0;

    ::SQLGetData(_hStmt, columnIndex, SQL_C_WCHAR, buf, sizeof(buf), &len);
    return buf;
}
