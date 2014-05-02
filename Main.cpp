#include "StdAfx.h"

#define __PROXY_CODE

#define PARAMETER_LEN_CHECK( num, msg, ... ) \
    if( i + num >= argc ) \
    { \
    printf(msg,__VA_ARGS__); \
    return 1; \
    }

char *UseEmptyString( char *string )
{
    return ( string ? string : "" );
}

int main( int argc, char* argv[] )
{
    int port = 8520;
    char remoteAddress[ 256 ] = { 0 };
    int remotePort = 6667;

    for( int i = 1; i < argc; ++i )
    {
        if( _strcmpi( argv[ i ], "-port" ) == 0 )
        {
            PARAMETER_LEN_CHECK( 1, "Port value was not supplied in parameters.\n" );

            port = atoi( argv[ ++i ] );
        }
        else if( _strcmpi( argv[ i ], "-server" ) == 0 )
        {
            PARAMETER_LEN_CHECK( 1, "Remote target address was not supplied in parameters.\n" );

            if( !SplitFullAddress( argv[ ++i ], remoteAddress, sizeof( remoteAddress ), &remotePort ) )
            {
                printf( "Invalid address provided.\n" );
                return 1;
            }
        }
    }

    printf( "Bounce (server port %d) --> %s:%d\n", port, remoteAddress, remotePort );

    WSADATA wd;
    WSAStartup( MAKEWORD( 2, 2 ), &wd );

    IrcUserinfo user;
    strcpy( user.szAccount, "my_bot" );
    strcpy( user.szNick, "Nicco_BNC" );
    strcpy( user.szName, "Nicco BNC" );

    CIrcClient client;

    client.SetUserinfo( &user );

    if( client.Connect( remoteAddress, remotePort ) )
    {
        printf( "Connection succeeded.\n" );

        while( client.IsConnected() )
        {
            client.Process();

            Sleep( 50 );
        }

        printf( "Connection closed.\n" );
    }
    else
        printf( "Connection failed to server.\n" );

    /*
        SOCKET sListen = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
        if( sListen != INVALID_SOCKET )
        {
        printf( "Created listening socket on %u\n", sListen );

        sockaddr_in addr;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_family = AF_INET;
        addr.sin_port = htons( port );

        if( bind( sListen, (LPSOCKADDR)&addr, sizeof( addr ) ) != SOCKET_ERROR &&
        listen( sListen, SOMAXCONN ) != SOCKET_ERROR )
        {
        printf( "Bound server to port %d\n", port );

        while( 1 )
        {
        printf( "Waiting for connection ...\n" );

        int addrlen = sizeof( addr );
        SOCKET c = accept( sListen, (LPSOCKADDR)&addr, &addrlen );
        if( c != INVALID_SOCKET )
        {
        char address[ 16 ];
        strcpy( address, inet_ntoa( addr.sin_addr ) );

        printf( "[%u] Client connected from %s\n", c, address );

        #ifdef __PROXY_CODE
        SOCKET s = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
        SetupSockaddr( &addr, remoteAddress, remotePort );

        if( connect( s, (LPSOCKADDR)&addr, sizeof( addr ) ) == SOCKET_ERROR )
        {
        printf( "Connection failed to remote host.\n" );
        closesocket( s );
        closesocket( c );
        break;
        }
        #endif // __PROXY_CODE

        #ifdef _DEBUG
        CLogger::GetInstance()->Write( "-- Session between %s (sck: %u) and %s:%d (sck: %u) has begun --", address, c, remoteAddress, remotePort, s );
        #endif // _DEBUG

        fd_set fd;
        TIMEVAL tv = { 0, 1 };
        char buffer[ 65536 ];

        while( 1 )
        {
        FD_ZERO( &fd );
        FD_SET( c, &fd );
        FD_SET( s, &fd );
        if( select( max( c, s ), &fd, NULL, NULL, &tv ) > 0 )
        {
        if( FD_ISSET( c, &fd ) ) // FROM IRC CLIENT
        {
        int len = recv( c, buffer, sizeof(buffer)-1, 0 );
        if( len > 0 )
        {
        printf( "[%u] Received %d bytes.\n", c, len );
        send( s, buffer, len, 0 );

        buffer[ len ] = 0;
        #ifdef _DEBUG
        CLogger::GetInstance()->Write( "[CLIENT]\t%s", buffer );
        #endif // _DEBUG
        }
        else
        {
        printf( "[%u] Disconnected.\n", c );
        break;
        }
        }
        else if( FD_ISSET( s, &fd ) ) // FROM IRC SERVER
        {
        int len = recv( s, buffer, sizeof(buffer)-1, 0 );
        if( len > 0 )
        {
        printf( "[%u] Received %d bytes.\n", s, len );
        send( c, buffer, len, 0 );

        buffer[ len ] = 0;
        #ifdef _DEBUG
        CLogger::GetInstance()->Write( "[SERVER]\t%s", buffer );
        #endif // _DEBUG
        }
        else
        {
        printf( "[%u] Disconnected.\n", s );
        break;
        }
        }
        }
        }

        closesocket( c );
        closesocket( s );
        }
        else
        printf( "Accept failed with error code: %d\n", WSAGetLastError() );
        }
        }
        else
        printf( "Failed to bind and listen on port %d\n", port );

        closesocket( sListen );
        }
        else
        printf( "Could not create listening socket.\n" );
        */

    Pause();
    WSACleanup();
    return 0;
}