#pragma once
#include <memory>
#include <mutex>
#include <event2/event.h>
#include <event2/buffer.h>

#include "net_peer.h"

struct bufferevent;

class NetConnection
{
public:
	NetConnection();
	~NetConnection();

	bool Initialize( NetPeer* _net_peer, evutil_socket_t _fd );
	void CreateConnection();
	void CloseConnection();
	NetPeer* GetNetPeer() const
	{
		return m_net_peer;
	}
	int32_t WriteData( evbuffer* _buff );
	const char* GetRemoteIp() const
	{
		return m_endpoint_address.address;
	}
	int32_t GetRemotePort() const
	{
		return m_endpoint_address.port;
	}

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
	void _onCreateConnection();
	void _onReceivedMessage( uint8_t* _data, uint32_t _length );
	void _onDestroyConnection();
	void _checkOutputData();

	void _raisedError();
private:
	NetPeer*				m_net_peer;
	bool					m_closing;
	bufferevent*			m_buffer_event;
	NetConnId				m_conn_id;
	NetStreamAddress		m_endpoint_address;
};

