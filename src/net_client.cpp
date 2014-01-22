#include "stdafx.h"
#include "net_client.h"

#include <event2/bufferevent.h>

#include "net_stream.h"
#include "net_connection.h"

CNetClient::CNetClient( CNetStream* _net_stream, const char* _address, uint16_t _port )
	: CNetPeer( _net_stream )
	, m_address( _address )
	, m_port( _port )
	, m_exit_event( nullptr )
{
}

CNetClient::~CNetClient()
{
}

bool CNetClient::Initialize()
{
	m_thread = new std::thread( std::ref( *this ) );
	return true;
}

void CNetClient::Stop()
{
	Terminate();
}

void CNetClient::Uninitialize()
{
	if( nullptr == m_thread )
		return;
	m_thread->join();
}

void CNetClient::operator()()
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

bool CNetClient::Load()
{
	if( nullptr != get_event_base() )
		return false;
	event_base* base = CreateEventBase();
	if( nullptr == base )
		return false;
	CNetConnection* conn = new CNetConnection();
	if( nullptr == conn )
		return false;
	if( !conn->Initialize( this, -1 ) )
	{
		delete conn;
		return false;
	}
	if( 0 != bufferevent_socket_connect_hostname( conn->get_buffer_event(), nullptr, AF_INET, m_address.c_str(), (int)m_port ) )
	{
		delete conn;
		return false;
	}
	m_exit_event = event_new( base, -1, 0, exit_connection_cb, this );
	return true;
}

void CNetClient::Run()
{
	event_base_dispatch( get_event_base() );
}

void CNetClient::Unload()
{
	get_net_stream()->CloseConnectionsOnPeer( get_peer_id() );
	DestroyEventBase();
}

void CNetClient::Terminate()
{
	if( m_execution_status >= eExecutionStatus::Terminating )
		return;
	event_active( m_exit_event, 0, 0 );
	m_execution_status = eExecutionStatus::Terminating;
}

void CNetClient::exit_connection_cb( evutil_socket_t /*_fd*/, short /*_what*/, void* _arg )
{
	CNetClient* client = (CNetClient*)_arg;
	event_base_loopexit( client->get_event_base(), nullptr );
}
