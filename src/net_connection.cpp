#include "stdafx.h"
#include "net_connection.h"

#ifdef WIN32
#include <Ws2ipdef.h>
#endif

#include <event2/bufferevent.h>

#include "net_peer.h"
#include "net_stream.h"

NetConnection::NetConnection()
	: m_net_peer( nullptr )
	, m_closing( false )
	, m_buffer_event( nullptr )
	, m_conn_id( 0 )
{
	memset( &m_endpoint_address, 0, sizeof( NetStreamAddress ) );
}

NetConnection::~NetConnection()
{
}

bool NetConnection::Initialize( NetPeer* _net_peer, evutil_socket_t _fd )
{
	NetConnId conn_id = _net_peer->get_net_stream()->BuildNewConnId();
	if( 0 == conn_id )
		return false;
	m_net_peer = _net_peer;
	m_conn_id = conn_id;
	m_buffer_event = bufferevent_socket_new( m_net_peer->get_event_base(), _fd, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE );
	bufferevent_setcb( m_buffer_event, read_cb, nullptr, event_cb, this );
	bufferevent_enable( m_buffer_event, EV_READ | EV_WRITE );
	if( -1 != _fd )
	{
		CreateConnection();
	}
	return true;
}

void NetConnection::_onCreateConnection()
{
	NetStreamPacket packet;
	packet.packet_type = MESSAGE_TYPE_CONNECTED;
	packet.peer_id = m_net_peer->get_peer_id();
	packet.net_conn_id = get_net_conn_id();
	packet.data = nullptr;
	packet.data_size = 0;
	m_net_peer->get_net_stream()->OnPacketArrived( packet );
}

void NetConnection::_onReceivedMessage( uint8_t* _data, uint32_t _length )
{
	NetStreamPacket packet;
	packet.packet_type = MESSAGE_TYPE_MESSAGE;
	packet.peer_id = m_net_peer->get_peer_id();
	packet.net_conn_id = get_net_conn_id();
	packet.data = _data;
	packet.data_size = _length;
	m_net_peer->get_net_stream()->OnPacketArrived( packet );
}

void NetConnection::_onDestroyConnection()
{
	NetStreamPacket packet;
	packet.packet_type = MESSAGE_TYPE_DISCONNECTED;
	packet.peer_id = m_net_peer->get_peer_id();
	packet.net_conn_id = get_net_conn_id();
	packet.data = nullptr;
	packet.data_size = 0;
	m_net_peer->get_net_stream()->OnPacketArrived( packet );

	bufferevent_free( m_buffer_event );
	m_buffer_event = nullptr;
	m_net_peer->get_net_stream()->DelConnection( get_net_conn_id() );
}

void NetConnection::CreateConnection()
{
	m_net_peer->get_net_stream()->AddConnection( this );

	// fill address
	memset( &m_endpoint_address, 0, sizeof( NetStreamAddress ) );
	evutil_socket_t fd = bufferevent_getfd( m_buffer_event );
	if( -1 != fd )
	{
		sockaddr_storage ss;
		ev_socklen_t len = ( ev_socklen_t )sizeof( sockaddr_storage );
		if( 0 ==  getpeername( bufferevent_getfd( m_buffer_event ), (sockaddr*)&ss, &len ) )
		{
			switch( ss.ss_family )
			{
			case AF_INET:
				evutil_inet_ntop( ss.ss_family, &( (sockaddr_in*)&ss )->sin_addr, m_endpoint_address.address, ADDRESS_LENGTH );
				m_endpoint_address.port = ( (sockaddr_in*)&ss )->sin_port;
				break;
			case AF_INET6:
				evutil_inet_ntop( ss.ss_family, &( (sockaddr_in6*)&ss )->sin6_addr, m_endpoint_address.address, ADDRESS_LENGTH );
				m_endpoint_address.port = ( (sockaddr_in6*)&ss )->sin6_port;
				break;
			}
		}
	}
	_onCreateConnection();
}

void NetConnection::CloseConnection()
{
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
		_onDestroyConnection();
	}
}

void NetConnection::_checkOutputData()
{
	if( !m_closing )
		return;
	if( nullptr == m_buffer_event )
		return;
	if( 0 != evbuffer_get_length( bufferevent_get_output( m_buffer_event ) ) )
		return;
	_onDestroyConnection();
}

int32_t NetConnection::WriteData( evbuffer* _buff )
{
	if( nullptr == m_buffer_event )
		return -1;
	if( m_closing )
		return -2;
	bufferevent_write_buffer( m_buffer_event, _buff );
	return 0;
}

#define MAX_ERR_MSG_LEN 1024
void NetConnection::_raisedError()
{
	char err_msg[MAX_ERR_MSG_LEN];
	snprintf( err_msg
		, MAX_ERR_MSG_LEN - 1
		, "Something goes wrong: %s"
		, evutil_socket_error_to_string( EVUTIL_SOCKET_ERROR() ) );
	err_msg[MAX_ERR_MSG_LEN - 1] = 0;
	m_net_peer->get_net_stream()->OnErrorMessage( m_net_peer->get_peer_id(), get_net_conn_id(), err_msg );
}

void NetConnection::read_cb( bufferevent* _bev, void *_ctx )
{
	evbuffer* input = bufferevent_get_input( _bev );
	NetConnection* net_connection = (NetConnection*)_ctx;
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
		net_connection->_onReceivedMessage( data, data_len );
	}
}

void NetConnection::write_cb( bufferevent* /*_bev*/, void *_ctx )
{
	NetConnection* net_connection = (NetConnection*)_ctx;
	net_connection->_checkOutputData();
}

void NetConnection::event_cb( bufferevent* /*_bev*/, short _events, void* _ctx )
{
	NetConnection* conn = (NetConnection*)_ctx;
	if( _events & ( BEV_EVENT_EOF | BEV_EVENT_ERROR ) )
	{
		if( _events & BEV_EVENT_ERROR )
			conn->_raisedError();
		conn->_onDestroyConnection();
	}
	else if( _events & BEV_EVENT_CONNECTED )
		conn->CreateConnection();
}
