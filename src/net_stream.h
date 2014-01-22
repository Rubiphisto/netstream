#pragma once
#include <memory>
#include <map>
#include <mutex>

#include "message_list.h"

class CNetPeer;
class CNetConnection;
struct NetStreamPacket;

typedef std::shared_ptr<CNetConnection> CNetConnectionPtr;

class CNetStream
{
public:
	CNetStream();
	~CNetStream();

	void Shutdown();
	NetPeerId StartServer( const char* _local_addr, uint16_t _port );
	NetPeerId ConnectServer( const char* _remote_addr, uint16_t _port );
	bool Disconnect( NetConnId _conn_id );
	bool ClosePeer( NetPeerId _peer_id );
	void AddNetPacket( NetStreamPacket& _packet );
	int32_t GetNetPacket( NetStreamPacket& _packet );
	int32_t SendNetMessage( NetConnId _conn_id, const void* _data, uint32_t _size );
	bool AddConnection( CNetConnection* _conn );
	void DelConnection( NetConnId _conn_id );
	bool GetConnection( NetConnId _conn_id, CNetConnectionPtr& _conn );
	void CloseConnectionsOnPeer( NetPeerId _peer_id );
	NetConnId BuildNewConnId();
	NetPeerId BuildNewPeerId();
private:
	bool CreateEventBase();
	void DestroyEventBase();
private:
	typedef std::map<NetPeerId, CNetPeer*> PeersList;
	//typedef std::map<NetConnId, CNetConnection*> ConnectionsList;
	typedef std::map<NetConnId, CNetConnectionPtr> ConnectionsList;
	PeersList			m_peers;
	ConnectionsList		m_connections;
	std::mutex			m_connection_mutex;
	NetConnId			m_max_conn_id;
	NetPeerId			m_max_peer_id;
	CMessageList		m_message_list;
};

