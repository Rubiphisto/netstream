#include "stdafx.h"
#include "net_stream.h"

#include <event2/thread.h>
#include <list>
#include <algorithm>

#include "net_peer.h"
#include "net_server.h"
#include "net_client.h"
#include "net_connection.h"

struct netstream : public CNetStream
{
	netstream( PacketArrivedHandler _handler, uint64_t _param )
		: CNetStream( _handler, _param )
	{
	}
};

netstream_t netstream_create( PacketArrivedHandler _handler, uint64_t _param )
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
	return new netstream( _handler, _param );
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

void netstream_destroy(netstream_t _net_stream)
{
	_net_stream->Shutdown();
	delete _net_stream;
}

void DefaultPacketArrivedHandler( netstream_t _net_stream, const NetStreamPacket& _packet, uint64_t )
{
	_net_stream->AddNetPacket( _packet );
}

CNetStream::CNetStream( PacketArrivedHandler _handler, uint64_t _param )
	: m_max_conn_id( 0 )
	, m_max_peer_id( 0 )
	, m_packet_arrived_handler( _handler )
	, m_param_with_handler( _param )
{
	if( nullptr == m_packet_arrived_handler )
		m_packet_arrived_handler = DefaultPacketArrivedHandler;
}

CNetStream::~CNetStream()
{
}

NetPeerId CNetStream::StartServer( const char* _local_addr, uint16_t _port )
{
	CNetServer* net_server = new CNetServer( this, _local_addr, _port );
	if( !net_server->Initialize() )
	{
		delete net_server;
		return 0;
	}
	m_peers.insert( std::make_pair( net_server->get_peer_id(), net_server ) );
	return net_server->get_peer_id();
}

NetPeerId CNetStream::ConnectServer( const char* _remote_addr, uint16_t _port )
{
	CNetClient* net_client = new CNetClient( this, _remote_addr, _port );
	if( !net_client->Initialize() )
	{
		delete net_client;
		return 0;
	}
	m_peers.insert( std::make_pair( net_client->get_peer_id(), net_client ) );
	return net_client->get_peer_id();
}

bool CNetStream::Disconnect( NetConnId _conn_id )
{
	CNetConnectionPtr connection;
	if( !GetConnection( _conn_id, connection ) )
		return false;
	connection->CloseConnection();
	return true;
}

bool CNetStream::ClosePeer( NetPeerId _peer_id )
{
	PeersList::iterator it = m_peers.find( _peer_id );
	if( it == m_peers.end() )
		return false;
	CNetPeer* peer = it->second;
	peer->Stop();
	peer->Uninitialize();
	delete peer;
	m_peers.erase( _peer_id );
	return true;
}

void CNetStream::Shutdown()
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
			CNetPeer* peer = _value.second;
			peer->Uninitialize();
			delete peer;
			return true;
		}
	);
	m_peers.clear();
}

void CNetStream::OnPacketArrived( const NetStreamPacket& _packet )
{
	m_packet_arrived_handler( (netstream_t)this, _packet, m_param_with_handler );
}

void CNetStream::AddNetPacket( const NetStreamPacket& _packet )
{
	m_message_list.PushMessage( _packet );
}

int32_t CNetStream::GetNetPacket( NetStreamPacket& _packet )
{
	bool ret = m_message_list.PopMessage( _packet );
	return ret ? 0 : -1;
}


int32_t CNetStream::SendNetMessage( NetConnId _conn_id, const void* _data, uint32_t _size )
{
	CNetConnectionPtr connection;
	if( !GetConnection( _conn_id, connection ) )
		return -1;
	return connection->WriteData( _data, _size );
}

bool CNetStream::AddConnection( CNetConnection* _conn )
{
	CNetConnectionPtr conn_ptr( _conn );
	std::lock_guard<std::mutex> lock( m_connection_mutex );
	m_connections[_conn->get_net_conn_id()] = conn_ptr;
	return true;
}

void CNetStream::DelConnection( NetConnId _conn_id )
{
	std::lock_guard<std::mutex> lock( m_connection_mutex );
	m_connections.erase( _conn_id );
}

bool CNetStream::GetConnection( NetConnId _conn_id, CNetConnectionPtr& _conn )
{
	_conn.reset();
	std::lock_guard<std::mutex> lock( m_connection_mutex );
	ConnectionsList::iterator it = m_connections.find( _conn_id );
	if( m_connections.end() == it )
		return false;
	_conn = it->second;
	return true;
}

void CNetStream::CloseConnectionsOnPeer( NetPeerId _peer_id )
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

NetConnId CNetStream::BuildNewConnId()
{
	return ++m_max_conn_id;
}

NetPeerId CNetStream::BuildNewPeerId()
{
	return ++m_max_peer_id;
}
