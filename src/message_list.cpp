#include "stdafx.h"

#include "message_list.h"

CMessageList::CMessageList()
{
}

CMessageList::~CMessageList()
{
}

void CMessageList::PushMessage( const NetStreamPacket& _packet )
{
	std::lock_guard<std::mutex> lock( m_mutex );
	m_messages.push( _packet );
}

bool CMessageList::PopMessage( NetStreamPacket& _packet )
{
	std::lock_guard<std::mutex> lock( m_mutex );
	if( m_messages.empty() )
		return false;
	_packet = m_messages.front();
	m_messages.pop();
	return true;
}
