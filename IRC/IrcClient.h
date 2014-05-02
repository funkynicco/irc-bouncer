#pragma once

struct IrcUserinfo
{
    char szAccount[ MAX_ACCOUNTNAME ];
    char szNick[ MAX_NICKNAME ];
    char szName[ MAX_NAME ];

    IrcUserinfo()
    {
        *szAccount = 0;
        *szNick = 0;
        *szName = 0;
    }

    BOOL IsValid()
    {
        return
            *szAccount &&
            *szNick;
    }
};

class CIrcClient
{
public:
    CIrcClient();
    virtual ~CIrcClient();
    BOOL Connect( const char* address, int port );
    void Close();
    void Process();

    inline BOOL IsConnected() { return m_socket != INVALID_SOCKET; }

    void SetUserinfo( IrcUserinfo* lpUserinfo );

protected:
    BOOL Send( const void* data, int length );
    BOOL SendFormat( const char* format, ... );

    virtual void OnConnect() { }
    virtual void OnDisconnect() { }
    virtual BOOL PreprocessData( const char* data, int length );
    virtual BOOL PreprocessPacket( const char* host, const char* header, const char* data );

private:
    CChannelManager m_channelManager;
    IrcUserinfo m_userinfo;
    SOCKET m_socket;
    fd_set m_fd;
    TIMEVAL m_tv;
    char m_buffer[ 65536 ];
    string m_databuf;
};