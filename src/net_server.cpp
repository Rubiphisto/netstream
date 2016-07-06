#include "stdafx.h"
#include "net_server.h"

#include <event2/listener.h>

#include "net_stream.h"
#include "net_connection.h"

NetServer::NetServer( NetStream* _net_stream, const char* _address, uint16_t _port )
	: NetPeer( _net_stream )
	, m_address( _address )
	, m_port( _port )
	, m_listener( nullptr )
{
	if( m_address.empty() )
		m_address = "[::]";
}

NetServer::~NetServer()
{
}

bool NetServer::_startService()
{
	if( nullptr != get_event_base() )
		return false;
	event_base* base = _createEventBase();
	if( nullptr == base )
		return false;
	sockaddr_storage addr;
	int32_t addr_len = sizeof( sockaddr_storage );
	char buff[512];
	evutil_snprintf( buff, 512, "%s:%u", m_address.c_str(), m_port );
	if( evutil_parse_sockaddr_port( buff, (sockaddr*)&addr, &addr_len ) )
		return false;

	m_listener = evconnlistener_new_bind( base
		, accept_conn_cb
		, this
		, LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE | LEV_OPT_THREADSAFE
		, -1
		, (sockaddr*)&addr
		, addr_len );
	if( nullptr == m_listener )
		return false;
	evconnlistener_set_error_cb( m_listener, accept_error_cb );
	return true;
}

void NetServer::_stopService()
{
	if( nullptr != m_listener )
	{
		evconnlistener_free( m_listener );
		m_listener = nullptr;
	}
	get_net_stream()->CloseConnectionsOnPeer( get_peer_id() );
	_destroyEventBase();
}

bool NetServer::_createConnection( event_base* _base, evutil_socket_t _fd )
{
	if( get_event_base() != _base )
		throw std::invalid_argument( "Unmatch event_base" );
	NetConnection* conn = new NetConnection();
	if( nullptr == conn )
		return false;
	if( !conn->Initialize( this, _fd ) )
	{
		delete conn;
		return false;
	}
	return true;
}

void NetServer::accept_conn_cb( evconnlistener * _listener
	, evutil_socket_t _fd
	, sockaddr* /*_addr*/
	, int32_t /*_socklen*/
	, void * _ctx )
{
	event_base* base = evconnlistener_get_base( _listener );
	NetServer* net_server = (NetServer*)_ctx;
	net_server->_createConnection( base, _fd );
}

#define MAX_ERR_MSG_LEN 1024
void NetServer::accept_error_cb( evconnlistener* /*_listener*/, void* _ctx )
{
	int32_t err = EVUTIL_SOCKET_ERROR();
	char err_msg[MAX_ERR_MSG_LEN];
	snprintf( err_msg
		, MAX_ERR_MSG_LEN - 1
		, "Got an error %" PRId32 " (%s) on the listener."
		, err
		, evutil_socket_error_to_string( err ) );
	err_msg[MAX_ERR_MSG_LEN - 1] = 0;
	NetServer* net_server = (NetServer*)_ctx;
	net_server->get_net_stream()->OnErrorMessage( net_server->get_peer_id(), 0, err_msg );
}
