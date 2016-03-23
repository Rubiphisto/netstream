#pragma once
#include <mutex>
#include <queue>
#include <thread>
#include <event2/event.h>

class NetStream;
class NetConnection;
struct event_base;
struct evbuffer;

enum eActionType
{
	/**
	Send message to remote
	*/
	Message,
	/**
	Break the connection with remote
	*/
	Disconnect,
	/**
	Close the peer
	*/
	Close,
};

struct Action
{
	NetConnId conn_id;
	eActionType action_type;
	evbuffer* buffer;
};

class NetPeer
{
public:
	/**
	Constructor
	*/
	NetPeer( NetStream* _net_stream );
	/**
	Destructor
	*/
	virtual ~NetPeer();
	/**
	Initialize the peer

	It actually open a new thread which provid the service
	*/
	bool Initialize();
	void Stop();
	void Uninitialize();
	void SendMsg( NetConnId _conn_id, const void* _data, uint32_t _size );
	void CloseConnection( NetConnId _conn_id );

	void operator()();

	NetStream* get_net_stream() const
	{
		return m_net_stream;
	}
	event_base* get_event_base() const
	{
		return m_event_base;
	}
	NetPeerId get_peer_id() const
	{
		return m_peer_id;
	}
protected:
	bool _startServiceInternal();
	void _stopServiceInternal();
	virtual bool _startService();
	virtual void _loopService();
	virtual void _stopService();

	event_base* _createEventBase();
	void _destroyEventBase();
	void _pushAction( const Action& _action );
	void _doAction();
	bool _getAction( Action& _action );

	static void action_cb( evutil_socket_t _fd, short _what, void* _arg );
private:
	NetPeerId			m_peer_id;
	std::thread*		m_thread;
	NetStream*			m_net_stream;
	event_base*			m_event_base;
	std::mutex			m_action_event_mutex;
	event*				m_action_event;
	std::mutex			m_actions_mutex;
	std::queue<Action>	m_actions;
protected:
	enum class eExecutionStatus
	{
		Scratch,
		Initialized,
		Running,
		Terminating,
		Terminated,
	};

	eExecutionStatus	m_execution_status;
};
