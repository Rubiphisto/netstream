#pragma once
#include "net_peer.h"

struct evconnlistener;

class NetServer : public NetPeer
{
public:
	/**
	the constructor
	*/
	NetServer( NetStream* _net_stream, const char* _address, uint16_t _port );
	/**
	the destructor
	*/
	~NetServer();
	// event's callback
	/**
	the libevent callback which is called when a new connection arrived
	*/
	static void accept_conn_cb( struct evconnlistener * _listener, evutil_socket_t _fd, sockaddr* _addr, int32_t _socklen, void * _ctx );
	/**
	the libevent callback which is called when error raised
	*/
	static void accept_error_cb( struct evconnlistener* _listener, void* _ctx );
protected:
	/**
	the implementation to start the service

	@return return true if the service is started successfully and vice versa.
	*/
	bool _startService() override;
	/**
	the implementation to stop the service
	*/
	void _stopService() override;
	/**
	*/
	bool _createConnection( event_base* _base, evutil_socket_t _fd );
private:
	/**
	the listen address
	*/
	std::string			m_address;
	/**
	the listen port
	*/
	uint16_t			m_port;
	/**
	the listener in libevent
	*/
	evconnlistener*		m_listener;
};

