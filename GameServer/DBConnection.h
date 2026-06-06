#pragma once

class DBConnection
{
public:
    explicit DBConnection(int id);
    ~DBConnection();

    bool Connect(const std::wstring& connectionString);
    void Disconnect();

    bool Execute(const std::wstring& query);

    bool Fetch();                        
    int GetInt(int columnIndex);            
    std::wstring GetString(int columnIndex); 

    bool IsConnected() const { return _connected; }
    int GetId() const { return _id; }

private:
    int _id = -1;
    bool _connected = false;
    std::wstring _connectionString;

    SQLHENV _hEnv = SQL_NULL_HENV; 
    SQLHDBC _hDbc = SQL_NULL_HDBC;
    SQLHSTMT _hStmt = SQL_NULL_HSTMT;
};
