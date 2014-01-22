#pragma once
#include <thread>
#include <map>
#include <event2/event.h>
#include <event2/listener.h>

#include "net_peer.h"

class CNetConnection;
struct event;

class CNetServer : public CNetPeer
{

public:
	CNetServer( CNetStream* _net_stream, const char* _address, uint16_t _port );
	~CNetServer();
	virtual bool Initialize();
	virtual void Stop();
	virtual void Uninitialize();
	void operator()();
	void Terminate();

public:
	// event's callback
	static void accept_conn_cb( struct evconnlistener * _listener, evutil_socket_t _fd, sockaddr* _addr, int32_t _socklen, void * _ctx );
	static void accept_error_cb( struct evconnlistener* _listener, void* _ctx );
	static void exit_listen_cb( evutil_socket_t _fd, short _what, void* _arg );
private:
	bool Load();
	void Run();
	void Unload();
	bool CreateConnection( event_base* _base, evutil_socket_t _fd );
private:
	std::thread*		m_thread;
	std::string			m_address;
	uint16_t			m_port;
	event*				m_exit_event;
	evconnlistener*		m_listener;
	//std::map<NetConnId, CNetConnection*>	m_connections;
};

