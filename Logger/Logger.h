#pragma once

class CLogger
{
public:
    CLogger();
    virtual ~CLogger();

    void Write( const char* format, ... );

    static CLogger* GetInstance()
    {
        static CLogger instance;
        return &instance;
    }

private:
    CRITICAL_SECTION m_cs;
    time_t m_tmInstanceId;
    char m_szFilename[ 256 ];
};

void ConvertToHex( const void* data, size_t sizeOfData, char* destination );
void LogData( BOOL bIsOutgoing, const void* data, size_t length );