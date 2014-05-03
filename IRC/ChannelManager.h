#pragma once

struct IrcChannelUser
{
    IrcChannelUser();

    char szNick[ MAX_NICKNAME ];
    BOOL bIsMe;
};

struct IrcChannel
{
    IrcChannel();
    ~IrcChannel();

    IrcChannelUser* AddUser( const char* name );
    BOOL RemoveUser( const char* name );
    BOOL IsMember( const char* name );

    char szName[ MAX_CHANNEL_NAME ];
    char szTopic[ MAX_TOPIC_NAME ];
    map<string, IrcChannelUser*> m_users;
};

class CChannelManager
{
public:
    CChannelManager();
    virtual ~CChannelManager();

    IrcChannel* AddChannel( const char* name );
    IrcChannel* GetChannel( const char* name );
    BOOL RemoveChannel( const char* name );

private:
    map<string, IrcChannel*> m_channels;
};