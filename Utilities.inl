#pragma once

inline void Pause()
{
    while( 1 )
    {
        if( _kbhit() )
        {
            int key = _getch();

            if( key == VK_ESCAPE )
                return;
        }

        Sleep( 100 );
    }
}

inline BOOL SplitFullAddress( const char* value, char* dnsOrIp, size_t sizeOfDnsOrIp, int* port, int defaultPort = 6667 )
{
    if( *value == NULL ||
        dnsOrIp == NULL ||
        port == NULL ||
        sizeOfDnsOrIp < 16 ) // should at LEAST support 255.255.255.255\0
        return FALSE;

    const char* ptr = value;
    const char* end = value + strlen( value );
    const char* addressEnd = NULL;

    *port = defaultPort;

    while( ptr < end )
    {
        if( *ptr == ':' )
        {
            if( size_t( ptr - value ) >= sizeOfDnsOrIp - 1 ) // make sure it fits in the provided buffer
                return FALSE;

            memcpy( dnsOrIp, value, ptr - value );
            dnsOrIp[ ptr - value ] = 0;
            ++ptr;
            *port = atoi( ptr );
            return TRUE;
        }

        ++ptr;
    }

    // there was no : in the string so we assume its just a basic dns or ip without port

    if( size_t( end - value ) >= sizeOfDnsOrIp - 1 ) // make sure it fits in the provided buffer
        return FALSE;

    strcpy( dnsOrIp, value );
    return TRUE;
}

inline void SetupSockaddr( LPSOCKADDR_IN lpAddr, const char* address, int port )
{
    LPHOSTENT lpHost = gethostbyname( address );
    if( lpHost )
        lpAddr->sin_addr.s_addr = *(u_long*)lpHost->h_addr_list[ 0 ];
    else
        lpAddr->sin_addr.s_addr = inet_addr( address );

    lpAddr->sin_family = AF_INET;
    lpAddr->sin_port = htons( port );
}