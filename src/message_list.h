#pragma once
#include <queue>
#include <mutex>

#include "libnetstream.h"

class CMessageList
{
public:
	CMessageList();
	~CMessageList();

	void PushMessage( const NetStreamPacket& _packet );
	bool PopMessage( NetStreamPacket& _packet );
private:
	std::mutex m_mutex;
	std::queue<NetStreamPacket>	m_messages;
};
