#include "stdafx.h"
#include "net_client.h"

#include <event2/bufferevent.h>

#include "net_stream.h"
#include "net_connection.h"

NetClient::NetClient( NetStream* _net_stream, const char* _address, uint16_t _port )
	: NetPeer( _net_stream )
	, m_address( _address )
	, m_port( _port )
{
}

NetClient::~NetClient()
{
}

bool NetClient::_startService()
{
	if( nullptr != get_event_base() )
		return false;
	event_base* base = _createEventBase();
	if( nullptr == base )
		return false;
	NetConnection* conn = new NetConnection();
	if( nullptr == conn )
		return false;
	if( !conn->Initialize( this, -1 ) )
	{
		delete conn;
		return false;
	}
	if( 0 != bufferevent_socket_connect_hostname( conn->get_buffer_event(), nullptr, AF_UNSPEC, m_address.c_str(), m_port ) )
	{
		delete conn;
		return false;
	}
	return true;
}

void NetClient::_stopService()
{
	get_net_stream()->CloseConnectionsOnPeer( get_peer_id() );
	_destroyEventBase();
}
