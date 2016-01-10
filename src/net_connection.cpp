#include "stdafx.h"
#include "net_connection.h"

#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>

#include "net_peer.h"
#include "net_stream.h"

CNetConnection::CNetConnection()
	: m_net_peer( nullptr )
	, m_closing( false )
	, m_buffer_event( nullptr )
	, m_conn_id( 0 )
{
}

CNetConnection::~CNetConnection()
{
}

bool CNetConnection::Initialize( CNetPeer* _net_peer, evutil_socket_t _fd )
{
	NetConnId conn_id = _net_peer->get_net_stream()->BuildNewConnId();
	if( 0 == conn_id )
		return false;
	m_net_peer = _net_peer;
	m_conn_id = conn_id;
	m_buffer_event = bufferevent_socket_new( m_net_peer->get_event_base(), _fd, BEV_OPT_CLOSE_ON_FREE );
	bufferevent_setcb( m_buffer_event, read_cb, nullptr, event_cb, this );
	bufferevent_enable( m_buffer_event, EV_READ | EV_WRITE );
	if( -1 != _fd )
	{
		CreateConnection();
	}
	return true;
}

void CNetConnection::OnCreateConnection()
{
	NetStreamPacket packet;
	packet.packet_type = MESSAGE_TYPE_CONNECTED;
	packet.peer_id = m_net_peer->get_peer_id();
	packet.net_conn_id = get_net_conn_id();
	packet.data = nullptr;
	packet.data_size = 0;
	m_net_peer->get_net_stream()->OnPacketArrived( packet );
}

void CNetConnection::OnReceivedMessage( uint8_t* _data, uint32_t _length )
{
	NetStreamPacket packet;
	packet.packet_type = MESSAGE_TYPE_MESSAGE;
	packet.peer_id = m_net_peer->get_peer_id();
	packet.net_conn_id = get_net_conn_id();
	packet.data = _data;
	packet.data_size = _length;
	m_net_peer->get_net_stream()->OnPacketArrived( packet );
}

void CNetConnection::OnDestroyConnection()
{
	NetStreamPacket packet;
	packet.packet_type = MESSAGE_TYPE_DISCONNECTED;
	packet.peer_id = m_net_peer->get_peer_id();
	packet.net_conn_id = get_net_conn_id();
	packet.data = nullptr;
	packet.data_size = 0;
	m_net_peer->get_net_stream()->OnPacketArrived( packet );

	{
		std::lock_guard<std::recursive_mutex> lock( m_bev_mutex );
		if( nullptr == m_buffer_event )
			return;
		bufferevent_free( m_buffer_event );
		m_buffer_event = nullptr;
	}
	m_net_peer->get_net_stream()->DelConnection( get_net_conn_id() );
}

void CNetConnection::CreateConnection()
{
	m_net_peer->get_net_stream()->AddConnection( this );
	OnCreateConnection();
}

void CNetConnection::CloseConnection()
{
	std::lock_guard<std::recursive_mutex> lock( m_bev_mutex );
	if( nullptr == m_buffer_event )
		return;
	if( m_closing )
		return;
	m_closing = true;
	if( 0 != evbuffer_get_length( bufferevent_get_output( m_buffer_event ) ) )
	{
		bufferevent_disable( m_buffer_event, EV_READ );
		bufferevent_setcb( m_buffer_event, nullptr, write_cb, event_cb, this );
	}
	else
	{
		OnDestroyConnection();
	}
}

void CNetConnection::CheckOutputData()
{
	std::lock_guard<std::recursive_mutex> lock( m_bev_mutex );
	if( !m_closing )
		return;
	if( nullptr == m_buffer_event )
		return;
	if( 0 != evbuffer_get_length( bufferevent_get_output( m_buffer_event ) ) )
		return;
	OnDestroyConnection();
}

int32_t CNetConnection::WriteData( const void* _data, uint32_t _size )
{
	std::lock_guard<std::recursive_mutex> lock( m_bev_mutex );
	if( nullptr == m_buffer_event )
		return -1;
	if( m_closing )
		return -2;
	uint32_t head = htonl( _size );
	bufferevent_write( m_buffer_event, &head, 4 );
	bufferevent_write( m_buffer_event, _data, _size );
	return 0;
}

#define MAX_ERR_MSG_LEN 1024
void CNetConnection::RaisedError()
{
	char err_msg[MAX_ERR_MSG_LEN];
	snprintf( err_msg
		, MAX_ERR_MSG_LEN - 1
		, "Something goes wrong: %s"
		, evutil_socket_error_to_string( EVUTIL_SOCKET_ERROR() ) );
	err_msg[MAX_ERR_MSG_LEN - 1] = 0;
	m_net_peer->get_net_stream()->OnErrorMessage( m_net_peer->get_peer_id(), get_net_conn_id(), err_msg );
}

void CNetConnection::read_cb( bufferevent* _bev, void *_ctx )
{
	evbuffer* input = bufferevent_get_input( _bev );
	CNetConnection* net_connection = (CNetConnection*)_ctx;
	while( true )
	{
		size_t buffer_len = evbuffer_get_length( input );
		if( buffer_len < 4 )
			return;
		ev_uint32_t data_len;
		evbuffer_copyout( input, &data_len, 4 );
		data_len = ntohl( data_len );
		if( buffer_len < data_len + 4 )
			return;
		uint8_t* data = new uint8_t[data_len];
		if( nullptr == data )
			return;
		evbuffer_drain( input, 4 );
		evbuffer_remove( input, data, data_len );
		net_connection->OnReceivedMessage( data, data_len );
	}
}

void CNetConnection::write_cb( bufferevent* /*_bev*/, void *_ctx )
{
	CNetConnection* net_connection = (CNetConnection*)_ctx;
	net_connection->CheckOutputData();
}

void CNetConnection::event_cb( bufferevent* /*_bev*/, short _events, void* _ctx )
{
	CNetConnection* conn = (CNetConnection*)_ctx;
	if( _events & ( BEV_EVENT_EOF | BEV_EVENT_ERROR ) )
	{
		if( _events & BEV_EVENT_ERROR )
			conn->RaisedError();
		conn->OnDestroyConnection();
	}
	else if( _events & BEV_EVENT_CONNECTED )
		conn->CreateConnection();
}
