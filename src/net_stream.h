#pragma once
#include <memory>
#include <map>
#include <mutex>

#include "message_list.h"

class NetPeer;
class NetConnection;
struct NetStreamPacket;

typedef std::shared_ptr<NetConnection> CNetConnectionPtr;

class NetStream
{
public:
	NetStream( PacketArrivedHandler _packet_handler, NetStreamErrorMsgHandler _errmsg_handler, uint64_t _param );
	~NetStream();

	void Shutdown();
	NetPeerId StartServer( const char* _local_addr, uint16_t _port );
	NetPeerId ConnectServer( const char* _remote_addr, uint16_t _port );
	bool Disconnect( NetConnId _conn_id );
	bool ClosePeer( NetPeerId _peer_id );
	void OnPacketArrived( const NetStreamPacket& _packet );
	void OnErrorMessage( NetPeerId _peer_id, NetConnId _conn_id, const char* _error_msg );
	void AddNetPacket( const NetStreamPacket& _packet );
	int32_t GetNetPacket( NetStreamPacket& _packet );
	int32_t SendNetMessage( NetConnId _conn_id, const void* _data, uint32_t _size );
	const char* GetRemoteIp( NetConnId _conn_id );
	int32_t GetRemotePort( NetConnId _conn_id );
	bool AddConnection( NetConnection* _conn );
	void DelConnection( NetConnId _conn_id );
	bool GetConnection( NetConnId _conn_id, CNetConnectionPtr& _conn );
	void CloseConnectionsOnPeer( NetPeerId _peer_id );
	NetConnId BuildNewConnId();
	NetPeerId BuildNewPeerId();
private:
	typedef std::map<NetPeerId, NetPeer*> PeersList;
	//typedef std::map<NetConnId, NetConnection*> ConnectionsList;
	typedef std::map<NetConnId, CNetConnectionPtr> ConnectionsList;
	PeersList			m_peers;
	ConnectionsList		m_connections;
	std::mutex			m_connection_mutex;
	NetConnId			m_max_conn_id;
	NetPeerId			m_max_peer_id;
	MessageList		m_message_list;
	PacketArrivedHandler		m_packet_arrived_handler;
	NetStreamErrorMsgHandler	m_errmsg_handler;
	uint64_t					m_param_with_handler;
};

