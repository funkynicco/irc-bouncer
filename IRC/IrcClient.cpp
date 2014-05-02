#include "..\StdAfx.h"
#include "IrcClient.h"

CIrcClient::CIrcClient() :
m_socket( INVALID_SOCKET )
{
    FD_ZERO( &m_fd );
    m_tv.tv_sec = 0;
    m_tv.tv_usec = 1;
}

CIrcClient::~CIrcClient()
{
    Close();
}

BOOL CIrcClient::Connect( const char* address, int port )
{
    if( m_socket != INVALID_SOCKET )
        return FALSE;

    if( ( m_socket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP ) ) == SOCKET_ERROR )
        return FALSE;

    sockaddr_in addr;
    SetupSockaddr( &addr, address, port );

    if( connect( m_socket, (LPSOCKADDR)&addr, sizeof( addr ) ) == SOCKET_ERROR )
    {
        closesocket( m_socket );
        m_socket = INVALID_SOCKET;
        return FALSE;
    }


    SendFormat(
        "NICK %s\r\nUSER %s 0 * :%s\r\n",
        m_userinfo.szNick,
        m_userinfo.szAccount,
        m_userinfo.szName );

    OnConnect();
    return TRUE;
}

void CIrcClient::Close()
{
    if( m_socket != INVALID_SOCKET )
    {
        closesocket( m_socket );
        m_socket = INVALID_SOCKET;

        OnDisconnect();
    }
}

void CIrcClient::Process()
{
    if( m_socket == INVALID_SOCKET &&
        !m_userinfo.IsValid() )
        return;

    FD_ZERO( &m_fd );
    FD_SET( m_socket, &m_fd );

    if( select( m_socket, &m_fd, NULL, NULL, &m_tv ) > 0 &&
        FD_ISSET( m_socket, &m_fd ) )
    {
        int len = recv( m_socket, m_buffer, sizeof( m_buffer ), 0 );
        if( len > 0 )
        {
            LogData( FALSE, m_buffer, len );
            PreprocessData( m_buffer, len );
        }
        else
            Close();
    }
}

BOOL CIrcClient::Send( const void* data, int length )
{
    if( m_socket == INVALID_SOCKET )
        return FALSE;

    int pos = 0;
    while( pos < length )
    {
        int sent = send( m_socket, (const char*)data + pos, length - pos, 0 );
        if( sent <= 0 )
            return FALSE;

        LogData( TRUE, (const char*)data + pos, sent );

        pos += sent;
    }

    return TRUE;
}

BOOL CIrcClient::SendFormat( const char* format, ... )
{
    static char message[ 4096 ];
    char* ptr = message;
    va_list l;
    va_start( l, format );
    int total = _vscprintf( format, l ) + 1; // +1 for including the \0

    if( total > sizeof( message ) ) // make sure that message can contain all the data we're about to write
        ptr = (char*)malloc( total );

    auto pos = vsprintf( ptr, format, l );
    va_end( l );

    BOOL bRes = Send( ptr, pos );

    if( ptr != message )
        free( ptr );

    return bRes;
}

BOOL CIrcClient::PreprocessData( const char* data, int length )
{
    // add to buffer and await ..

    string packet, content, host;

    m_databuf.append( data, length );

    size_t pos = string::npos;
    while( ( pos = m_databuf.find( "\n" ) ) != string::npos )
    {
        string packet = m_databuf.substr( 0, pos );
        m_databuf = m_databuf.substr( pos + 1 );

        if( packet.length() > 0 )
        {
            if( *( packet.c_str() + packet.length() - 1 ) == '\r' ) // remove ending \r (if exist)
            {
                packet = packet.substr( 0, packet.length() - 1 );
                if( packet.length() == 0 )
                    continue;
            }

            // :SillyBot!SillyBot@46-116-168-232.bb.netvision.net.il QUIT :Ping timeout: 121 seconds\r\n
            // :penguin.omega.org.za
            if( packet[ 0 ] == ':' )
            {
                if( ( pos = packet.find( " " ) ) == string::npos )
                    continue;

                host = packet.substr( 1, pos - 1 );

                packet = packet.substr( pos + 1 );
                if( packet.length() == 0 )
                    continue;
            }

            string content;
            if( ( pos = packet.find( " " ) ) != string::npos )
            {
                content = packet.substr( pos + 1 );
                packet = packet.substr( 0, pos );
            }

            if( !PreprocessPacket( host.c_str(), packet.c_str(), content.c_str() ) )
            {
                // do something ...
            }
        }
    }

    return TRUE;
}

BOOL CIrcClient::PreprocessPacket( const char* host, const char* header, const char* data )
{
    char data_small[ 256 ];
    size_t data_len = strlen( data );
    if( data_len > 50 )
    {
        data_len = 50;
        memcpy( data_small, data, data_len );
        data_small[ data_len ] = 0;
        strcat( data_small, "..." );
    }
    else
        strcpy( data_small, data );

    string host_x = host;
    size_t pos = host_x.find( "@" );
    if( pos != string::npos )
        host_x = host_x.substr( 0, pos );

    if( ( pos = host_x.find( "!" ) ) != string::npos )
        host_x = host_x.substr( 0, pos );

    printf( "HST(%s), HDR(%s), DT(%s)\n", host_x.c_str(), header, data_small );
    CLogger::GetInstance()->Write( "[RECV-PACKET] Header(%s), Data(%s)\n", header, data );

    if( _strcmpi( header, "001" ) == 0 )
    {
        SendFormat( "JOIN #test\r\n" );
    }
    else if( _strcmpi( header, "PING" ) == 0 )
    {
        SendFormat( "PONG %s\r\n", data );
    }

    return TRUE;
}

void CIrcClient::SetUserinfo( IrcUserinfo* lpUserinfo )
{
    strcpy( m_userinfo.szAccount, lpUserinfo->szAccount );
    strcpy( m_userinfo.szNick, lpUserinfo->szNick );
    strcpy( m_userinfo.szName, lpUserinfo->szName );
}