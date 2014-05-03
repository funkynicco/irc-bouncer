#pragma once

interface IIrcClientCallback
{
    virtual void OnConnect() = 0;
    virtual void OnDisconnect() = 0;
    virtual void OnPacket( const string& nick, const string& user, const string& host, const string& header, string data ) = 0;
};

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
    inline void SetCallback( IIrcClientCallback* pCallback ) { m_pCallback = pCallback; }

    void SetUserinfo( IrcUserinfo* lpUserinfo );
    inline const IrcUserinfo* GetUserinfo() { return &m_userinfo; }

    inline CChannelManager* GetChannelManager() { return &m_channelManager; }

    BOOL Send( const void* data, int length );
    BOOL SendFormat( const char* format, ... );

protected:
    virtual void OnConnect() { }
    virtual void OnDisconnect() { }
    virtual BOOL PreprocessData( const char* data, int length );
    virtual BOOL PreprocessPacket( const char* host, const char* header, const char* data );

    CChannelManager m_channelManager;
    IrcUserinfo m_userinfo;

private:
    IIrcClientCallback* m_pCallback;
    SOCKET m_socket;
    fd_set m_fd;
    TIMEVAL m_tv;
    char m_buffer[ 65536 ];
    string m_databuf;
};