#include "stdafx.h"
#include "net_server.h"

#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/event.h>
#include <string.h>
#include <exception>

#include "net_stream.h"
#include "net_connection.h"

CNetServer::CNetServer( CNetStream* _net_stream, const char* _address, uint16_t _port )
	: CNetPeer( _net_stream )
	, m_address( _address )
	, m_port( _port )
	, m_listener( nullptr )
{
}

CNetServer::~CNetServer()
{
}

bool CNetServer::Initialize()
{
	m_thread = new std::thread( std::ref( *this ) );
	return true;
}

void CNetServer::Stop()
{
	Terminate();
}

void CNetServer::Uninitialize()
{
	if( nullptr == m_thread )
		return;
	m_thread->join();
}

void CNetServer::operator()()
{
	if( Load() )
	{
		m_execution_status = eExecutionStatus::Running;
		Run();
		m_execution_status = eExecutionStatus::Terminating;
		Unload();
	}
	else
	{
		m_execution_status = eExecutionStatus::Terminating;
		Unload();
		// TODO: There is no method to notify the fail situation to the outer.
	}
	m_execution_status = eExecutionStatus::Terminated;
}

bool CNetServer::Load()
{
	if( nullptr != get_event_base() )
		return false;
	event_base* base = CreateEventBase();
	if( nullptr == base )
		return false;
	sockaddr_in addr;
	memset( &addr, 0, sizeof( sockaddr_in ) );
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl( 0 );
	addr.sin_port = htons( m_port );

	m_listener = evconnlistener_new_bind( base
		, accept_conn_cb
		, this
		, LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE
		, -1
		, (sockaddr*)&addr
		, sizeof( sockaddr_in ) );
	if( nullptr == m_listener )
		return false;
	evconnlistener_set_error_cb( m_listener, accept_error_cb );
	m_exit_event = event_new( base, -1, 0, exit_listen_cb, this );
	return true;
}

void CNetServer::Run()
{
	event_base_dispatch( get_event_base() );
}

void CNetServer::Unload()
{
	evconnlistener_free( m_listener );
	m_listener = nullptr;
	get_net_stream()->CloseConnectionsOnPeer( get_peer_id() );
	event_free( m_exit_event );
	m_exit_event = nullptr;
	DestroyEventBase();
}

void CNetServer::Terminate()
{
	if( m_execution_status >= eExecutionStatus::Terminating )
		return;
	event_active( m_exit_event, 0, 0 );
	m_execution_status = eExecutionStatus::Terminating;
}

bool CNetServer::CreateConnection( event_base* _base, evutil_socket_t _fd )
{
	if( get_event_base() != _base )
		throw std::invalid_argument( "Unmatch event_base" );
	CNetConnection* conn = new CNetConnection();
	if( nullptr == conn )
		return false;
	if( !conn->Initialize( this, _fd ) )
	{
		delete conn;
		return false;
	}
	//m_connections.insert( std::make_pair( peer->get_net_conn_id(), peer ) );
	return true;
}

void CNetServer::accept_conn_cb( evconnlistener * _listener
	, evutil_socket_t _fd
	, sockaddr* /*_addr*/
	, int32_t /*_socklen*/
	, void * _ctx )
{
	event_base* base = evconnlistener_get_base( _listener );
	CNetServer* net_server = (CNetServer*)_ctx;
	net_server->CreateConnection( base, _fd );
}

void CNetServer::accept_error_cb( evconnlistener* /*_listener*/, void* /*_ctx*/ )
{
	//event_base* base = evconnlistener_get_base( _listener );
	int32_t err = EVUTIL_SOCKET_ERROR();
	fprintf( stderr
		, "Got an error %d (%s) on the listener.\n"
		, err
		, evutil_socket_error_to_string( err ) );
	//event_base_loopexit( base, nullptr );
}

void CNetServer::exit_listen_cb( evutil_socket_t /*_fd*/, short /*_what*/, void* _arg )
{
	CNetServer* server = (CNetServer*)_arg;
	event_base_loopexit( server->get_event_base(), nullptr );
}

