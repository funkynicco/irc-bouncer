#pragma once

#include <iostream>
#include <WinSock2.h>
#include <Windows.h>

#pragma comment( lib, "ws2_32.lib" )

#include <conio.h>
#include <time.h>

#include <string>
#include <map>
using namespace std;

#include "Utilities.inl"
#include "Logger\Logger.h"
#include "IRC\IRCStandards.h"
#include "IRC\ChannelManager.h"
#include "IRC\IrcClient.h"
#include "IRC\IrcBouncer.h"