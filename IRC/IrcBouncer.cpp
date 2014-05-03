#include "..\StdAfx.h"
#include "IrcBouncer.h"

CIrcBouncer::CIrcBouncer() :
m_listenSock( INVALID_SOCKET ),
m_userSock( INVALID_SOCKET ),
m_tmNextPing( 0 ),
m_tmSentPing( 0 )
{
    m_client.SetCallback( this );
    FD_ZERO( &m_fd );
    m_tv.tv_sec = 0;
    m_tv.tv_usec = 1;
}

CIrcBouncer::~CIrcBouncer()
{
    Close();
}

BOOL CIrcBouncer::StartServer( int port )
{
    if( m_listenSock != INVALID_SOCKET )
        return FALSE;

    if( ( m_listenSock = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP ) ) == INVALID_SOCKET )
        return FALSE;

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons( port );

    if( bind( m_listenSock, (LPSOCKADDR)&addr, sizeof( addr ) ) == SOCKET_ERROR ||
        listen( m_listenSock, SOMAXCONN ) == SOCKET_ERROR )
        return FALSE;

    return TRUE;
}

void CIrcBouncer::Close()
{
    if( m_listenSock != INVALID_SOCKET )
    {
        closesocket( m_listenSock );
        m_listenSock = INVALID_SOCKET;
    }

    if( m_userSock != INVALID_SOCKET )
    {
        closesocket( m_userSock );
        m_userSock = INVALID_SOCKET;
    }

    m_client.Close();
}

void CIrcBouncer::Process()
{
    m_client.Process();

    if( m_listenSock != INVALID_SOCKET )
    {
        // process accepting

        FD_ZERO( &m_fd );
        FD_SET( m_listenSock, &m_fd );
        if( select( m_listenSock, &m_fd, NULL, NULL, &m_tv ) > 0 &&
            FD_ISSET( m_listenSock, &m_fd ) )
        {
            sockaddr_in addr;
            int addrlen = sizeof( addr );

            SOCKET sck = accept( m_listenSock, (LPSOCKADDR)&addr, &addrlen );
            if( sck == INVALID_SOCKET )
            {
                printf( __FUNCTION__ " - accept returned INVALID_SOCKET (code: %u)\n", WSAGetLastError() );
                return;
            }

            if( m_userSock == INVALID_SOCKET )
            {
                strcpy( m_userIp, inet_ntoa( addr.sin_addr ) );

                // TODO: check if allowed to connect

                m_userSock = sck;
                Process_OnUserConnect();
            }
            else
            {
                const char* response = ":" NETWORK_NAME " NOTICE Auth :*** Multiple connections are not allowed.\r\n";
                send( sck, response, strlen( response ), 0 ); // this is a blocking call so we can safely closesocket after this call
                closesocket( sck );
                printf( "BNC socket %u (%s) rejected, a client already connected!\n", sck, inet_ntoa( addr.sin_addr ) );
            }
        }
    }

    if( m_userSock != INVALID_SOCKET )
    {
        // process connected user

        FD_ZERO( &m_fd );
        FD_SET( m_userSock, &m_fd );
        if( select( m_userSock, &m_fd, NULL, NULL, &m_tv ) > 0 &&
            FD_ISSET( m_userSock, &m_fd ) )
        {
            int len = recv( m_userSock, m_buffer, sizeof( m_buffer ), 0 );
            if( len > 0 )
            {
                string packet, content;

                m_databuf.append( m_buffer, len ); // TODO: move this to a standalone class that can be shared between Bouncer and IRC Client

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

                        string content;
                        if( ( pos = packet.find( " " ) ) != string::npos )
                        {
                            content = packet.substr( pos + 1 );
                            packet = packet.substr( 0, pos );
                        }

                        Process_OnUserData( packet, content );
                    }
                }
            }
            else
            {
                closesocket( m_userSock );
                Process_OnUserDisconnect();
                m_userSock = INVALID_SOCKET;
                *m_userIp = 0;
            }
        }

        time_t tmNow = time( NULL );

        if( m_tmSentPing > 0 &&
            tmNow - m_tmSentPing > 30 )
        {
            printf( "[ERR] Ping timeout from client\n" );

            closesocket( m_userSock );
            Process_OnUserDisconnect();
            m_userSock = INVALID_SOCKET;
            *m_userIp = 0;

            return;
        }

        if( tmNow >= m_tmNextPing )
        {
            SendFormat( "PING :TIMEOUTCHECK\r\n" );
            if( m_tmSentPing == 0 )
                m_tmSentPing = tmNow;
            m_tmNextPing = tmNow + 60;
        }
    }
}

void CIrcBouncer::Process_OnUserConnect()
{
    m_tmNextPing = time( NULL ) + 60;

    printf( "BNC user connected from %s (sck: %u)\n", m_userIp, m_userSock );

    SendFormat( ":%s NOTICE Auth :*** You're now connected to \2IRC BNC Server\2.\r\n", NETWORK_NAME );
}

void CIrcBouncer::Process_OnUserDisconnect()
{
    printf( "BNC user disconnected from %s (sck: %u)\n", m_userIp, m_userSock );
}

void CIrcBouncer::Process_OnUserData( const string& header, string data )
{
    printf( "[BNC-%u] HDR(%s), DATA(%s)\n", m_userSock, header.c_str(), data.c_str() );

    if( header == "NICK" )
    {
        if( data.compare( m_client.GetUserinfo()->szNick ) != 0 )
        {
            printf( "BNC user sent invalid nick in nick request (%s, required: %s)\n", data.c_str(), m_client.GetUserinfo()->szNick );
            Close();
        }
        else
        {
            SendFormat( ":%s 001 %s :Successfully signed on.\r\n", NETWORK_NAME, data.c_str() );
            SendFormat( ":%s 372 %s :Message of the day\r\n", NETWORK_NAME, data.c_str() );
            SendFormat( ":%s 376 %s :End Motd\r\n", NETWORK_NAME, data.c_str() );

            SendFormat( ":%s!%s@hostname.com JOIN :#test\r\n", m_client.GetUserinfo()->szNick, "banana", "lol.com" );
            auto channel = m_client.GetChannelManager()->GetChannel( "test" );
            if( channel )
            {
                string pack = ":" NETWORK_NAME " 353 ";
                pack += m_client.GetUserinfo()->szNick;
                pack += " = #test :";

                for( map<string, IrcChannelUser*>::iterator it = channel->m_users.begin(); it != channel->m_users.end(); ++it )
                {
                    pack += it->second->szNick;
                    pack += " ";
                }

                pack += "\r\n:" NETWORK_NAME " 366 ";
                pack += m_client.GetUserinfo()->szNick;
                pack += " #test :End of /NAMES list.\r\n";
                Send( pack.c_str(), (int)pack.size() );

                if( *channel->szTopic )
                    SendFormat( ":%s 332 %s #%s :%s\r\n", NETWORK_NAME, m_client.GetUserinfo()->szNick, channel->szName, channel->szTopic );
            }
        }
    }
    else if( header == "PRIVMSG" )
    {
        //PRIVMSG #test :msg
        size_t pos = data.find( " :" );
        if( pos != string::npos )
        {
            string channel = data.substr( 0, pos );
            const char* ptrChannel = channel.c_str();
            if( *ptrChannel &&
                *ptrChannel == '#' )
                ++ptrChannel;

            data = data.substr( pos + 2 );

            auto ch = m_client.GetChannelManager()->GetChannel( ptrChannel );
            if( ch )
            {
                m_client.SendFormat( "PRIVMSG #%s :%s\r\n", ptrChannel, data.c_str() );
                printf( "[%s --> #%s] %s\n", m_client.GetUserinfo()->szNick, ptrChannel, data.c_str() );
            }
            else
            {
                printf( "BNC invalid channel #%s in a PRIVMSG request from user\n", ptrChannel );
            }
        }
    }
    else if( header == "PONG" )
    {
        m_tmSentPing = 0; // reset
    }
}

void CIrcBouncer::OnConnect()
{
    // the irc client connected to the IRC server
}

void CIrcBouncer::OnDisconnect()
{
    // we lost connection to the IRC server
    printf( "Lost connection to IRC server\n" );
    Close();
}

void CIrcBouncer::OnPacket( const string& nick, const string& user, const string& host, const string& header, string data )
{
    if( header == "PRIVMSG" )
    {
        size_t pos = data.find( " :" );
        if( pos != string::npos )
        {
            string channel = data.substr( 0, pos );
            const char* ptrChannel = channel.c_str();
            if( *ptrChannel &&
                *ptrChannel == '#' )
                ++ptrChannel;

            data = data.substr( pos + 2 );

            SendFormat( ":%s!%s@%s PRIVMSG #%s :%s\r\n", nick.c_str(), user.c_str(), host.c_str(), ptrChannel, data.c_str() );
            printf( "[%s --> #%s] %s\n", nick.c_str(), ptrChannel, data.c_str() );
        }
    }
}

BOOL CIrcBouncer::Send( const void* data, int length )
{
    if( m_userSock == INVALID_SOCKET )
        return FALSE;

    int pos = 0;
    while( pos < length )
    {
        int sent = send( m_userSock, (const char*)data + pos, length - pos, 0 );
        if( sent <= 0 )
            return FALSE;

        LogData( TRUE, (const char*)data + pos, sent );

        pos += sent;
    }

    return TRUE;
}

BOOL CIrcBouncer::SendFormat( const char* format, ... )
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