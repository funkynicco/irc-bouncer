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
            //LogData( FALSE, m_buffer, len );
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

    string x_nick, x_account, x_host;

    string host_x = host;
    size_t pos = host_x.find( "@" ); // strip away hostname
    if( pos != string::npos )
    {
        x_host = host_x.substr( pos + 1 );
        host_x = host_x.substr( 0, pos );
    }

    if( ( pos = host_x.find( "!" ) ) != string::npos ) // strip away account name
    {
        x_account = host_x.substr( pos + 1 );
        host_x = host_x.substr( 0, pos );
    }

    if( x_host.length() == 0 )
        x_host = host_x;
    else
        x_nick = host_x;

    //printf( "HST(%s), HDR(%s), DT(%s)\n", host_x.c_str(), header, data_small );
    if( x_nick.length() > 0 &&
        x_account.length() > 0 )
        CLogger::GetInstance()->Write( "[RECV-PACKET] Host(%s!%s@%s), Header(%s), Data(%s)\n", x_nick.c_str(), x_account.c_str(), x_host.c_str(), header, data );
    else
        CLogger::GetInstance()->Write( "[RECV-PACKET] Host(%s), Header(%s), Data(%s)\n", x_host.c_str(), header, data );

    if( _strcmpi( header, "001" ) == 0 )
    {
        //SendFormat( "JOIN #flyffrepublic\r\n" );
        SendFormat( "JOIN #test\r\n" );
    }
    else if( _strcmpi( header, "PING" ) == 0 )
    {
        SendFormat( "PONG %s\r\n", data );
    }
    else if( _strcmpi( header, "JOIN" ) == 0 )
    {
        // ... joined a channel waw
        if( x_nick.compare( m_userinfo.szNick ) == 0 ) // myself
        {
            // we've essentially joined the channel here

            if( strlen( data ) < 3 )
                return FALSE;

            auto channel = m_channelManager.AddChannel( data + 2 );
            if( channel == NULL )
            {
                printf( __FUNCTION__ " m_channelManager.AddChannel returned null! (channel #%s)\n", data + 2 );
                return FALSE;
            }

            channel->AddUser( m_userinfo.szNick )->bIsMe = TRUE;

            printf( "You joined '#%s'\n", data + 2 );
        }
        else
        {
            // someone else joined a channel we are in
            auto channel = m_channelManager.GetChannel( data + 2 );
            if( channel == NULL )
            {
                printf( __FUNCTION__ " m_channelManager.GetChannel returned null! (channel #%s)\n", data + 2 );
                return FALSE;
            }

            if( !channel->AddUser( x_nick.c_str() ) )
            {
                printf( __FUNCTION__ " Channel AddUser returned false! (channel #%s, user %s)\n", data + 2, x_nick.c_str() );
                return FALSE;
            }

            printf( "'%s' joined '#%s'\n", x_nick.c_str(), data + 2 );
        }
    }
    else if( _strcmpi( header, "PART" ) == 0 )
    {
        size_t dataLen = strlen( data );
        string channelName = dataLen > 0 ? ( data + 1 ) : data;
        size_t pos = channelName.find( " " );
        if( pos != string::npos )
            channelName = channelName.substr( 0, pos );

        auto channel = m_channelManager.GetChannel( channelName.c_str() );
        if( channel == NULL )
        {
            printf( __FUNCTION__ " m_channelManager.GetChannel returned null! (channel #%s)\n", channelName.c_str() );
            return FALSE;
        }

        if( !channel->RemoveUser( x_nick.c_str() ) )
        {
            printf( __FUNCTION__ " Channel RemoveUser returned false! (channel #%s, user %s)\n", channelName.c_str(), x_nick.c_str() );
            return FALSE;
        }

        printf( "'%s' left '#%s'\n", x_nick.c_str(), channelName.c_str() );
    }
    else if( _strcmpi( header, "353" ) == 0 )
    {
        string x_data = data;

        size_t pos = x_data.find( " = " );
        if( pos == string::npos )
        {
            printf( __FUNCTION__ " - Malformed userlist received from server.\n" );
            return FALSE;
        }

        x_data = x_data.substr( pos + 3 ); // #test :Yarin Nicco_BNC Nicco

        if( ( pos = x_data.find( " :" ) ) == string::npos )
        {
            printf( __FUNCTION__ " - Malformed userlist received from server.\n" );
            return FALSE;
        }

        string channelName = x_data.substr( 0, pos );
        if( channelName.length() < 2 )
        {
            printf( __FUNCTION__ " - Malformed userlist received from server.\n" );
            return FALSE;
        }

        const char* channelPtr = channelName.c_str() + 1;

        auto channel = m_channelManager.GetChannel( channelPtr );
        if( channel == NULL )
        {
            printf( __FUNCTION__ " - Could not get channel '#s' in userlist\n", channelPtr );
            return FALSE;
        }

        x_data = x_data.substr( pos + 2 );
        while( ( pos = x_data.find( " " ) ) != string::npos )
        {
            string name = x_data.substr( 0, pos );
            x_data = x_data.substr( pos + 1 );
            if( name.length() > 0 &&
                name != m_userinfo.szNick ) // dont add myself
            {
                if( !channel->AddUser( name.c_str() ) )
                {
                    printf( __FUNCTION__ " - Could not add '%s' to channel '#%s' in userlist request\n", name.c_str(), channelPtr );
                    return FALSE;
                }

                printf( "Member of '#%s': '%s'\n", channelPtr, name.c_str() );
            }
        }
    }
    else if( _strcmpi( header, "366" ) == 0 ) // End of /NAMES list
    {

    }
    else if( _strcmpi( header, "PRIVMSG" ) == 0 )
    {
        string msg = data;
        string channelName;

        size_t pos = msg.find( " :" );
        if( pos == string::npos )
        {
            printf( __FUNCTION__ " - Malformed PRIVMSG\n" );
            return FALSE;
        }

        channelName = msg.substr( 0, pos );
        msg = msg.substr( pos + 2 );

        if( channelName.length() < 2 )
        {
            printf( __FUNCTION__ " - Malformed channel name in PRIVMSG.\n" );
            return FALSE;
        }

        const char* channelPtr = channelName.c_str() + 1;

        printf( "[#%s] <%s> %s\n", channelPtr, x_nick.c_str(), msg.c_str() );
    }

    return TRUE;
}

void CIrcClient::SetUserinfo( IrcUserinfo* lpUserinfo )
{
    strcpy( m_userinfo.szAccount, lpUserinfo->szAccount );
    strcpy( m_userinfo.szNick, lpUserinfo->szNick );
    strcpy( m_userinfo.szName, lpUserinfo->szName );
}