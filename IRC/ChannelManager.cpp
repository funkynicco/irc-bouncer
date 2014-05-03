#include "..\StdAfx.h"
#include "ChannelManager.h"

/*
 * IrcChannelUser
 */

IrcChannelUser::IrcChannelUser() :
bIsMe( FALSE )
{
    *szNick = 0;
}

/*
 * IrcChannel
 */

IrcChannel::IrcChannel()
{
    *szName = 0;
    *szTopic = 0;
}

IrcChannel::~IrcChannel()
{
    for( map<string, IrcChannelUser*>::iterator it = m_users.begin(); it != m_users.end(); ++it )
        delete it->second;
}

IrcChannelUser* IrcChannel::AddUser( const char* name )
{
    if( strlen( name ) >= MAX_NICKNAME )
        return NULL;

    char _temp[ MAX_NICKNAME ];
    strcpy( _temp, name );
    _strlwr( _temp );

    if( m_users.find( _temp ) != m_users.end() )
        return NULL;

    auto user = new IrcChannelUser;
    strcpy( user->szNick, name );
    m_users.insert( pair<string, IrcChannelUser*>( _temp, user ) );
    return user;
}

BOOL IrcChannel::RemoveUser( const char* name )
{
    if( strlen( name ) >= MAX_NICKNAME )
        return NULL;

    char _temp[ MAX_NICKNAME ];
    strcpy( _temp, name );
    _strlwr( _temp );

    map<string, IrcChannelUser*>::iterator it = m_users.find( _temp );
    if( it == m_users.end() )
        return FALSE;

    delete it->second;
    m_users.erase( it );
    return TRUE;
}

BOOL IrcChannel::IsMember( const char* name )
{
    if( strlen( name ) >= MAX_NICKNAME )
        return NULL;

    char _temp[ MAX_NICKNAME ];
    strcpy( _temp, name );
    _strlwr( _temp );

    return m_users.find( _temp ) != m_users.end();
}

/*
 * CChannelManager
 */

CChannelManager::CChannelManager()
{

}

CChannelManager::~CChannelManager()
{
    for( map<string, IrcChannel*>::iterator it = m_channels.begin(); it != m_channels.end(); ++it )
        delete it->second;
}

IrcChannel* CChannelManager::AddChannel( const char* name )
{
    if( strlen( name ) >= MAX_CHANNEL_NAME )
        return NULL;

    char _temp[ MAX_CHANNEL_NAME ];
    strcpy( _temp, name );
    _strlwr( _temp );

    if( m_channels.find( _temp ) != m_channels.end() )
        return NULL;

    auto channel = new IrcChannel;
    strcpy( channel->szName, name );
    m_channels.insert( pair<string, IrcChannel*>( _temp, channel ) );
    return channel;
}

IrcChannel* CChannelManager::GetChannel( const char* name )
{
    if( strlen( name ) >= MAX_CHANNEL_NAME )
        return NULL;

    char _temp[ MAX_CHANNEL_NAME ];
    strcpy( _temp, name );
    _strlwr( _temp );

    map<string, IrcChannel*>::iterator it = m_channels.find( _temp );
    return it != m_channels.end() ? it->second : NULL;
}

BOOL CChannelManager::RemoveChannel( const char* name )
{
    if( strlen( name ) >= MAX_CHANNEL_NAME )
        return FALSE;

    char _temp[ MAX_CHANNEL_NAME ];
    strcpy( _temp, name );
    _strlwr( _temp );

    map<string, IrcChannel*>::iterator it = m_channels.find( _temp );
    if( it == m_channels.end() )
        return FALSE;

    delete it->second;
    m_channels.erase( it );
    return TRUE;
}