#include "stdafx.h"
#include "net_peer.h"

#include <event2/event.h>
#include <event2/buffer.h>

#include "net_stream.h"
#include "net_connection.h"

NetPeer::NetPeer( NetStream* _net_stream )
	: m_peer_id( _net_stream->BuildNewPeerId() )
	, m_thread( nullptr )
	, m_net_stream( _net_stream )
	, m_event_base( nullptr )
	, m_action_event( nullptr )
	, m_execution_status( eExecutionStatus::Scratch )
{
}

NetPeer::~NetPeer()
{
}

bool NetPeer::Initialize()
{
	m_thread = new std::thread( std::ref( *this ) );
	return true;
}

void NetPeer::Stop()
{
	if( m_execution_status >= eExecutionStatus::Terminating )
		return;
	Action action;
	action.action_type = eActionType::Close;
	action.buffer = nullptr;
	action.conn_id = 0;
	_pushAction( action );
	m_execution_status = eExecutionStatus::Terminating;
}

void NetPeer::Uninitialize()
{
	if( nullptr == m_thread )
		return;
	m_thread->join();
	delete m_thread;
	m_thread = nullptr;
}

void NetPeer::SendMsg( NetConnId _conn_id, const void* _data, uint32_t _size )
{
	Action action;
	action.action_type = eActionType::Message;
	action.buffer = evbuffer_new();
	evbuffer_expand( action.buffer, _size + sizeof( uint32_t ) );
	uint32_t head = htonl( _size );
	evbuffer_add( action.buffer, &head, sizeof( uint32_t ) );
	evbuffer_add( action.buffer, _data, _size );
	action.conn_id = _conn_id;
	_pushAction( action );
}

void NetPeer::CloseConnection( NetConnId _conn_id )
{
	Action action;
	action.action_type = eActionType::Disconnect;
	action.buffer = nullptr;
	action.conn_id = _conn_id;
	_pushAction( action );
}

void NetPeer::operator()()
{
	if( _startServiceInternal() )
	{
		m_execution_status = eExecutionStatus::Running;
		_loopService();
		m_execution_status = eExecutionStatus::Terminating;
		_stopServiceInternal();
	}
	else
	{
		m_execution_status = eExecutionStatus::Terminating;
		_stopServiceInternal();
		// TODO: There is no method to notify the fail situation to the outer.
	}
	m_execution_status = eExecutionStatus::Terminated;
}

bool NetPeer::_startServiceInternal()
{
	if( !_startService() )
		return false;
	{
		std::lock_guard<std::mutex> lock( m_action_event_mutex );
		m_action_event = event_new( m_event_base, -1, 0, action_cb, this );
		event_active( m_action_event, 0, 0 );
	}
	return true;
}

void NetPeer::_stopServiceInternal()
{
	{
		std::lock_guard<std::mutex> lock( m_action_event_mutex );
		event_free( m_action_event );
		m_action_event = nullptr;
	}
	_stopService();
}

bool NetPeer::_startService()
{
	return false;
}

void NetPeer::_loopService()
{
	event_base_dispatch( get_event_base() );
}

void NetPeer::_stopService()
{
}

event_base* NetPeer::_createEventBase()
{
	event_config* evt_config = event_config_new();
	//event_config_set_flag( evt_config, EVENT_BASE_FLAG_STARTUP_IOCP );
	m_event_base = event_base_new_with_config( evt_config );
	event_config_free( evt_config );
	return m_event_base;
}

void NetPeer::_destroyEventBase()
{
	if( nullptr != m_event_base )
	{
		event_base_free( m_event_base );
		m_event_base = nullptr;
	}
}

void NetPeer::_pushAction( const Action& _action )
{
	{
		std::lock_guard<std::mutex> lock( m_actions_mutex );
		m_actions.push( _action );
	}
	{
		std::lock_guard<std::mutex> lock( m_action_event_mutex );
		if( nullptr == m_action_event )
			return;
		event_active( m_action_event, 0, 0 );
	}
}

bool NetPeer::_getAction( Action& _action )
{
	std::lock_guard<std::mutex> lock( m_actions_mutex );
	if( m_actions.empty() )
		return false;
	_action = m_actions.front();
	m_actions.pop();
	return true;
}

void NetPeer::_doAction()
{
	Action action;
	while( _getAction( action ) )
	{
		switch( action.action_type )
		{
		case eActionType::Message:
		{
			CNetConnectionPtr connection;
			if( !m_net_stream->GetConnection( action.conn_id, connection ) )
				continue;
			connection->WriteData( action.buffer );
			evbuffer_free( action.buffer );
			break;
		}
		case eActionType::Disconnect:
		{
			CNetConnectionPtr connection;
			if( !m_net_stream->GetConnection( action.conn_id, connection ) )
				continue;
			connection->CloseConnection();
			break;
		}
		case eActionType::Close:
			event_base_loopexit( get_event_base(), nullptr );
			break;
		}
	}
}

void NetPeer::action_cb( evutil_socket_t /*_fd*/, short /*_what*/, void* _arg )
{
	NetPeer* peer = (NetPeer*)_arg;
	peer->_doAction();
}
