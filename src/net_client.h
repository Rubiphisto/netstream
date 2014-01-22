#pragma once 
#include <thread>
#include <event2/event.h>

#include "net_peer.h"

struct bufferevent;
struct event;

class CNetClient : public CNetPeer
{
public:
	CNetClient( CNetStream* _net_stream, const char* _address, uint16_t _port );
	~CNetClient();

	virtual bool Initialize();
	virtual void Stop();
	virtual void Uninitialize();

	void operator()();
public:
	static void exit_connection_cb( evutil_socket_t _fd, short _what, void* _arg );
private:
	bool Load();
	void Run();
	void Unload();
	void Terminate();
private:
	std::thread*		m_thread;
	std::string			m_address;
	uint16_t			m_port;
	event*				m_exit_event;
};
