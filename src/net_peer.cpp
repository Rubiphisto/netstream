#include "stdafx.h"
#include "net_peer.h"

#include <event2/event.h>

#include "net_stream.h"


CNetPeer::CNetPeer( CNetStream* _net_stream )
	: m_net_stream( _net_stream )
	, m_event_base( nullptr )
	, m_execution_status( eExecutionStatus::Scratch )
	, m_peer_id( _net_stream->BuildNewPeerId() )
{
}

CNetPeer::~CNetPeer()
{
}

event_base* CNetPeer::CreateEventBase()
{
	event_config* evt_config = event_config_new();
	//event_config_set_flag( evt_config, EVENT_BASE_FLAG_STARTUP_IOCP );
	m_event_base = event_base_new_with_config( evt_config );
	event_config_free( evt_config );
	return m_event_base;
}

void CNetPeer::DestroyEventBase()
{
	if( nullptr != m_event_base )
	{
		event_base_free( m_event_base );
		m_event_base = nullptr;
	}
}

