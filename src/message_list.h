#pragma once
#include <queue>
#include <mutex>

#include "libnetstream.h"

class MessageList
{
public:
	MessageList();
	~MessageList();

	void PushMessage( const NetStreamPacket& _packet );
	bool PopMessage( NetStreamPacket& _packet );
private:
	std::mutex m_mutex;
	std::queue<NetStreamPacket>	m_messages;
};
