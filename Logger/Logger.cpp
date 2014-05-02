#include "..\StdAfx.h"
#include "Logger.h"

CLogger::CLogger()
{
    InitializeCriticalSectionAndSpinCount( &m_cs, 2000 );
    m_tmInstanceId = time( NULL );
    sprintf( m_szFilename, "Log\\Instance_%I64u.txt", (unsigned __int64)m_tmInstanceId );

    CreateDirectory( "Log", NULL );
}

CLogger::~CLogger()
{
    DeleteCriticalSection( &m_cs );
}

void CLogger::Write( const char* format, ... )
{
    EnterCriticalSection( &m_cs );

    static char message[ 4096 ];
    char* ptr = message;
    va_list l;
    va_start( l, format );
    int total = _vscprintf( format, l ) + 1; // +1 for including the \0

    // [2014-02-01 11:29:52]\r\n
    total += 25; // size of above ^, including the ending \r\n at the end of the ENTIRE message (everything)

    if( total > sizeof( message ) ) // make sure that message can contain all the data we're about to write
        ptr = (char*)malloc( total );

    SYSTEMTIME st;
    GetLocalTime( &st );
    int p = sprintf( ptr, "[%04d-%02d-%02d %02d:%02d:%02d]\r\n", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond );

    p += vsprintf( ptr + p, format, l );
    va_end( l );

    // append the ending new line
    memcpy( ptr + p, "\r\n", 2 );

    HANDLE hFile = CreateFile( m_szFilename, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
    if( hFile != INVALID_HANDLE_VALUE )
    {
        SetFilePointer( hFile, 0, NULL, FILE_END ); // set the file pointer to the end (append)

        DWORD dwWritten;
        DWORD dwPos = 0;
        DWORD dwSize = total - 1; // do not include the ending \0
        while( dwPos < dwSize )
        {
            if( !WriteFile( hFile, ptr + dwPos, dwSize - dwPos, &dwWritten, NULL ) )
            {
                printf( __FUNCTION__ " - Error occured while writing file: %d\n", GetLastError() );
                break;
            }

            dwPos += dwWritten;
        }
        CloseHandle( hFile );
    }
    else
        printf( __FUNCTION__ " - Error opening '%s': %d\n", m_szFilename, GetLastError() );

    if( ptr != message )
        free( ptr );

    LeaveCriticalSection( &m_cs );
}

inline void ConvertToHex( const unsigned char cb, char* dst )
{
    const char* hexvalues = "0123456789ABCDEF";
    *dst++ = hexvalues[ ( cb >> 4 ) & 0x0f ];
    *dst++ = hexvalues[ cb & 0x0f ];
    *dst = 0;
}

void ConvertToHex( const void* data, size_t sizeOfData, char* destination )
{
    const char* hexvalues = "0123456789ABCDEF";

    const unsigned char* ucdata = (const unsigned char*)data;

    for( size_t i = 0; i < sizeOfData; ++i )
    {
        *destination++ = hexvalues[ ( ucdata[ i ] >> 4 ) & 0x0f ];
        *destination++ = hexvalues[ ucdata[ i ] & 0x0f ];
    }

    *destination = 0;
}

inline BOOL IsPrintable( char cb )
{
    if( isalpha( cb ) ||
        isdigit( cb ) )
        return TRUE;

    switch( cb )
    {
    case '.':
    case ' ':
    case ':':
    case '*':
    case '(':
    case ')':
    case '-':
    case '_':
    case ',':
    case ';':
    case '?':
    case '!':
    case '#':
    case '"':
    case '\'':
    case '/':
    case '\\':
    case '=':
    case '+':
    case '&':
    case '%':
    case '@':
        return TRUE;
    }

    return FALSE;
}

void LogData( BOOL bIsOutgoing, const void* data, size_t length )
{
    char hex[ 3 ];

    static char _data[ 16384 ];
    char* ptr = _data;

    for( size_t i = 0; i < length; ++i )
    {
        const unsigned char cb = static_cast<const unsigned char*>( data )[ i ];
        if( !IsPrintable( cb ) )
        {
            ConvertToHex( cb, hex );

            *ptr++ = '[';
            memcpy( ptr, hex, 2 ); ptr += 2;
            *ptr++ = ']';
        }
        else
            *ptr++ = (char)cb;
    }

    *ptr = 0;

    CLogger::GetInstance()->Write( "[%s] %s", bIsOutgoing ? "OUT" : "IN", _data );
}