#include "stdafx.h"
#include "net_stream.h"

#include <event2/thread.h>
#include <list>
#include <algorithm>

#include "net_peer.h"
#include "net_server.h"
#include "net_client.h"
#include "net_connection.h"

struct netstream : public NetStream
{
	netstream( PacketArrivedHandler _packet_handler, NetStreamErrorMsgHandler _errmsg_handler, uint64_t _param )
		: NetStream( _packet_handler, _errmsg_handler, _param )
	{
	}
};

netstream_t netstream_create( PacketArrivedHandler _packet_handler, NetStreamErrorMsgHandler _errmsg_handler, uint64_t _param )
{
#ifdef WIN32
	uint16_t version = MAKEWORD( 2, 2 );
	WSADATA WSAData;
	WSAStartup( version, &WSAData );
#endif
#if defined(WIN32)
	evthread_use_windows_threads();
#else
	evthread_use_pthreads();
#endif
	return new netstream( _packet_handler, _errmsg_handler, _param );
}

NetPeerId netstream_listen( netstream_t _net_stream, const char* _local_addr, uint16_t _port )
{
	return _net_stream->StartServer( _local_addr, _port );
}

NetPeerId netstream_connect( netstream_t _net_stream, const char* _remote_addr, uint16_t _port )
{
	return _net_stream->ConnectServer( _remote_addr, _port );
}

bool netstream_disconnect( netstream_t _net_stream, NetConnId _conn_id )
{
	return _net_stream->Disconnect( _conn_id );
}

bool netstream_close( netstream_t _net_stream, NetPeerId _peer_id )
{
	return _net_stream->ClosePeer( _peer_id );
}

int32_t netstream_recv( netstream_t _net_stream, NetStreamPacket& _packet )
{
	return _net_stream->GetNetPacket( _packet );
}

void netstream_free_packet( NetStreamPacket* _packet )
{
	if( MESSAGE_TYPE_MESSAGE == _packet->packet_type )
	{
		delete[] _packet->data;
		_packet->data = nullptr;
		_packet->data_size = 0;
	}
}

int32_t netstream_send( netstream_t _net_stream, NetConnId _conn_id, const void* _data, uint32_t _size )
{
	return _net_stream->SendNetMessage( _conn_id, _data, _size );
}

void netstream_destroy( netstream_t _net_stream )
{
	_net_stream->Shutdown();
	delete _net_stream;
}

void DefaultPacketArrivedHandler( netstream_t _net_stream, const NetStreamPacket& _packet, uint64_t )
{
	_net_stream->AddNetPacket( _packet );
}

void DefaultErrorMsgHandler( netstream_t /*_net_stream*/, NetPeerId _peer_id, NetConnId _conn_id, const char* _error_msg, uint64_t )
{
	printf( "[netstream][PeerId:%u|ConnId:%llu]%s\n", _peer_id, _conn_id, _error_msg );
}

NetStream::NetStream( PacketArrivedHandler _packet_handler, NetStreamErrorMsgHandler _errmsg_handler, uint64_t _param )
	: m_max_conn_id( 0 )
	, m_max_peer_id( 0 )
	, m_packet_arrived_handler( _packet_handler )
	, m_errmsg_handler( _errmsg_handler )
	, m_param_with_handler( _param )
{
	if( nullptr == m_packet_arrived_handler )
		m_packet_arrived_handler = DefaultPacketArrivedHandler;
	if( nullptr == m_errmsg_handler )
		m_errmsg_handler = DefaultErrorMsgHandler;
}

NetStream::~NetStream()
{
}

NetPeerId NetStream::StartServer( const char* _local_addr, uint16_t _port )
{
	NetServer* net_server = new NetServer( this, _local_addr, _port );
	if( !net_server->Initialize() )
	{
		delete net_server;
		return 0;
	}
	m_peers.insert( std::make_pair( net_server->get_peer_id(), net_server ) );
	return net_server->get_peer_id();
}

NetPeerId NetStream::ConnectServer( const char* _remote_addr, uint16_t _port )
{
	NetClient* net_client = new NetClient( this, _remote_addr, _port );
	if( !net_client->Initialize() )
	{
		delete net_client;
		return 0;
	}
	m_peers.insert( std::make_pair( net_client->get_peer_id(), net_client ) );
	return net_client->get_peer_id();
}

bool NetStream::Disconnect( NetConnId _conn_id )
{
	CNetConnectionPtr connection;
	if( !GetConnection( _conn_id, connection ) )
		return false;
	connection->GetNetPeer()->CloseConnection( _conn_id );
	//connection->CloseConnection();
	return true;
}

bool NetStream::ClosePeer( NetPeerId _peer_id )
{
	PeersList::iterator it = m_peers.find( _peer_id );
	if( it == m_peers.end() )
		return false;
	NetPeer* peer = it->second;
	peer->Stop();
	peer->Uninitialize();
	delete peer;
	m_peers.erase( _peer_id );
	return true;
}

void NetStream::Shutdown()
{
	std::for_each( m_peers.begin(), m_peers.end(),
		[]( const PeersList::value_type& _value )
	{
		_value.second->Stop();
		return true;
	}
	);
	std::for_each( m_peers.begin(), m_peers.end(),
		[]( const PeersList::value_type& _value )
	{
		NetPeer* peer = _value.second;
		peer->Uninitialize();
		delete peer;
		return true;
	}
	);
	m_peers.clear();
}

void NetStream::OnPacketArrived( const NetStreamPacket& _packet )
{
	m_packet_arrived_handler( ( netstream_t )this, _packet, m_param_with_handler );
}

void NetStream::OnErrorMessage( NetPeerId _peer_id, NetConnId _conn_id, const char* _error_msg )
{
	m_errmsg_handler( ( netstream_t )this, _peer_id, _conn_id, _error_msg, m_param_with_handler );
}

void NetStream::AddNetPacket( const NetStreamPacket& _packet )
{
	m_message_list.PushMessage( _packet );
}

int32_t NetStream::GetNetPacket( NetStreamPacket& _packet )
{
	bool ret = m_message_list.PopMessage( _packet );
	return ret ? 0 : -1;
}


int32_t NetStream::SendNetMessage( NetConnId _conn_id, const void* _data, uint32_t _size )
{
	CNetConnectionPtr connection;
	if( !GetConnection( _conn_id, connection ) )
		return -1;
	connection->GetNetPeer()->SendMsg( _conn_id, _data, _size );
	return 0;
}

bool NetStream::AddConnection( NetConnection* _conn )
{
	CNetConnectionPtr conn_ptr( _conn );
	std::lock_guard<std::mutex> lock( m_connection_mutex );
	m_connections[_conn->get_net_conn_id()] = conn_ptr;
	return true;
}

void NetStream::DelConnection( NetConnId _conn_id )
{
	std::lock_guard<std::mutex> lock( m_connection_mutex );
	m_connections.erase( _conn_id );
}

bool NetStream::GetConnection( NetConnId _conn_id, CNetConnectionPtr& _conn )
{
	_conn.reset();
	std::lock_guard<std::mutex> lock( m_connection_mutex );
	ConnectionsList::iterator it = m_connections.find( _conn_id );
	if( m_connections.end() == it )
		return false;
	_conn = it->second;
	return true;
}

void NetStream::CloseConnectionsOnPeer( NetPeerId _peer_id )
{
	std::list<CNetConnectionPtr> list;
	{ // lock guard
		std::lock_guard<std::mutex> lock( m_connection_mutex );
		std::for_each( m_connections.begin(), m_connections.end(),
			[&list, _peer_id]( const ConnectionsList::value_type& _value )
		{
			CNetConnectionPtr connection = _value.second;
			if( connection->get_net_peer_id() == _peer_id )
				list.push_back( connection );
		}
		);
	}
	std::for_each( list.begin(), list.end(),
		[]( CNetConnectionPtr _conn )
	{
		_conn->CloseConnection();
	}
	);
}

NetConnId NetStream::BuildNewConnId()
{
	int64_t now_time = time( nullptr );
	++m_max_conn_id;
	return ( ( now_time & 0x0FFFFFFFF ) << 32 ) | ( m_max_conn_id & 0x0FFFFFFFF );
}

NetPeerId NetStream::BuildNewPeerId()
{
	return ++m_max_peer_id;
}
