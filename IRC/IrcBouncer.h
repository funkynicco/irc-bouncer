#pragma once

// This class works like a proxy
class CIrcBouncer : public IIrcClientCallback
{
public:
    CIrcBouncer();
    virtual ~CIrcBouncer();

    inline BOOL Connect( const char* address, int port ) { return m_client.Connect( address, port ); }
    BOOL StartServer( int port );
    void Close();
    void Process();

    inline BOOL IsClientConnected() { return m_userSock != INVALID_SOCKET; }
    inline BOOL IsConnectedToServer() { return m_client.IsConnected(); }
    inline void SetUserinfo( IrcUserinfo* lpUserinfo ) { m_client.SetUserinfo( lpUserinfo ); }

private:
    void OnConnect(); // BNC <--> IRC Server
    void OnDisconnect(); // BNC <--> IRC Server
    void OnPacket( const string& nick, const string& user, const string& host, const string& header, string data ); // BNC <--> IRC Server

    void Process_OnUserConnect(); // User <--> BNC
    void Process_OnUserDisconnect(); // User <--> BNC
    void Process_OnUserData( const string& header, string data ); // User <--> BNC

    BOOL Send( const void* data, int length ); // BNC --> User
    BOOL SendFormat( const char* format, ... ); // BNC --> User

    CIrcClient m_client;
    fd_set m_fd;
    TIMEVAL m_tv;
    SOCKET m_listenSock, m_userSock;
    char m_buffer[ 65536 ];
    char m_userIp[ 16 ];
    string m_databuf;
    time_t m_tmNextPing;
    time_t m_tmSentPing;
};