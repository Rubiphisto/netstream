#pragma once
#include <memory>
#include <mutex>
#include <event2/event.h>

#include "net_peer.h"

struct bufferevent;

class CNetConnection
{
public:
	CNetConnection();
	~CNetConnection();

	bool Initialize( CNetPeer* _net_peer, evutil_socket_t _fd );
	void CreateConnection();
	void CloseConnection();
	int32_t WriteData( const void* _data, uint32_t _size );

	// property
	NetConnId get_net_conn_id() const
	{
		return m_conn_id;
	}
	NetPeerId get_net_peer_id() const
	{
		return m_net_peer->get_peer_id();
	}
	bufferevent* get_buffer_event() const
	{
		return m_buffer_event;
	}
public:
	static void read_cb( struct bufferevent* _bev, void *_ctx );
	static void write_cb( struct bufferevent* _bev, void *_ctx );
	static void event_cb( struct bufferevent* _bev, short _events, void* _ctx );
private:
	void OnCreateConnection();
	void OnReceivedMessage( uint8_t* _data, uint32_t _length );
	void OnDestroyConnection();
	void CheckOutputData();

	void RaisedError();
private:
	CNetPeer*				m_net_peer;
	bool					m_closing;
	std::recursive_mutex	m_bev_mutex;
	bufferevent*			m_buffer_event;
	NetConnId				m_conn_id;
};

