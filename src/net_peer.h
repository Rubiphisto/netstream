#pragma once

class CNetStream;
struct event_base;

class CNetPeer
{
public:
	CNetPeer( CNetStream* _net_stream );
	virtual ~CNetPeer();

	virtual bool Initialize() = 0;
	virtual void Stop() = 0;
	virtual void Uninitialize() = 0;

	CNetStream* get_net_stream() const
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
	event_base* CreateEventBase();
	void DestroyEventBase();
private:
	NetPeerId			m_peer_id;
	CNetStream*			m_net_stream;
	event_base*			m_event_base;
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
