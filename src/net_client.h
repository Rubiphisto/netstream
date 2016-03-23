#pragma once 
#include "net_peer.h"

class NetClient : public NetPeer
{
public:
	/**
	the constructor
	*/
	NetClient( NetStream* _net_stream, const char* _address, uint16_t _port );
	/**
	the destructor
	*/
	~NetClient();
private:
	/**
	the implementation to start the service

	@return return true if the service is started successfully and vice versa.
	*/
	bool _startService() override;
	/**
	the implementation to stop the service
	*/
	void _stopService() override;
private:
	/**
	the host address
	*/
	std::string			m_address;
	/**
	the host's listening port
	*/
	uint16_t			m_port;
};
