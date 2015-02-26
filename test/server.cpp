#include <stdint.h>
#include <conio.h>
#include <list>
#include <iostream>
#include <algorithm>
#include <thread>
#include <queue>
#include <string>
#include <mutex>
#include "libnetstream.h"

#if defined( _WINDOWS )
#include <windows.h>
#endif

bool g_running = true;
std::list<NetConnId> all_connections;
std::mutex g_list_mutex;
std::queue<std::string> g_sending_list;

template<int MaxLen = 256>
class formater
{
	char	mBuffer[MaxLen];
public:
	formater( const char* format, ... )
	{
		va_list	va;
		va_start( va, format );
		if( -1 == vsnprintf( mBuffer, MaxLen - 1, format, va ) )
			std::cout << "[formater]: Buffer too small! " << std::endl;
		mBuffer[MaxLen - 1] = 0;
		va_end( va );
	}
	operator char*( )
	{
		return mBuffer;
	}
	operator const char*( ) const
	{
		return mBuffer;
	}
};
class CAnyString
{
public:
	CAnyString()
	{
	}
	CAnyString( const CAnyString& right )
	{
		mData = right.mData;
	}
	CAnyString& operator=( const CAnyString& _other )
	{
		mData = _other.mData;
		return *this;
	}
	CAnyString( const unsigned char* v )
	{
		if( nullptr == v )
			mData.clear();
		else
			mData = (const char*)v;
	}
	CAnyString( const char* v )
	{
		if( nullptr == v )
			mData.clear();
		else
			mData = v;
	}
	CAnyString( const std::string& v )
	{
		mData = v;
	}
	CAnyString( uint8_t v )
	{
		mData = formater<32>( "%u", v );
	}
	CAnyString( int16_t v )
	{
		mData = formater<32>( "%d", v );
	}
	CAnyString( uint16_t v )
	{
		mData = formater<32>( "%u", v );
	}
	CAnyString( int32_t v )
	{
		mData = formater<32>( "%d", v );
	}
	CAnyString( uint32_t v )
	{
		mData = formater<32>( "%u", v );
	}
	CAnyString( int64_t v )
	{
		mData = formater<32>( "%I64d", v );
	}
	CAnyString( uint64_t v )
	{
		mData = formater<32>( "%I64u", v );
	}
	CAnyString( DWORD v )
	{
		mData = formater<32>( "%u", v );
	}
	CAnyString( float v )
	{
		mData = formater<32>( "%f", v );
	}
	CAnyString( double v )
	{
		mData = formater<32>( "%e", v );
	}
	CAnyString( bool v )
	{
		mData = v ? "true" : "false";
	}
	// Assignment
	CAnyString& operator=( uint8_t v )
	{
		mData = formater<32>( "%u", v ); return *this;
	}
	CAnyString& operator=( int16_t v )
	{
		mData = formater<32>( "%d", v ); return *this;
	}
	CAnyString& operator=( uint16_t v )
	{
		mData = formater<32>( "%u", v ); return *this;
	}
	CAnyString& operator=( int32_t v )
	{
		mData = formater<32>( "%d", v ); return *this;
	}
	CAnyString& operator=( uint32_t v )
	{
		mData = formater<32>( "%u", v ); return *this;
	}
	CAnyString& operator=( int64_t v )
	{
		mData = formater<32>( "%I64d", v ); return *this;
	}
	CAnyString& operator=( uint64_t v )
	{
		mData = formater<32>( "%I64u", v ); return *this;
	}
	CAnyString& operator=( DWORD v )
	{
		mData = formater<32>( "%u", v ); return *this;
	}
	CAnyString& operator=( float v )
	{
		mData = formater<32>( "%f", v ); return *this;
	}
	CAnyString& operator=( double v )
	{
		mData = formater<32>( "%e", v ); return *this;
	}
	CAnyString& operator=( const char* v )
	{
		mData = v; return *this;
	}
	CAnyString& operator=( const unsigned char* v )
	{
		mData = (const char*)v; return *this;
	}
	CAnyString& operator=( const std::string& v )
	{
		mData = v; return *this;
	}
	CAnyString& operator=( bool v )
	{
		mData = v ? "true" : "false"; return *this;
	}
	// Type-cast
	//operator int8_t() const { return (int8_t)atoi( mData.c_str() ); }
	operator uint8_t() const
	{
		return (uint8_t)atoi( mData.c_str() );
	}
	operator int16_t() const
	{
		return (int16_t)atoi( mData.c_str() );
	}
	operator uint16_t() const
	{
		return (uint16_t)atoi( mData.c_str() );
	}
	operator int32_t() const
	{
		return (int32_t)atoi( mData.c_str() );
	}
	operator uint32_t() const
	{
		return (uint32_t)atoi( mData.c_str() );
	}
	operator int64_t() const
	{
		return (int64_t)_strtoi64( mData.c_str(), 0, 0 );
	}
	operator uint64_t() const
	{
		return (uint64_t)_strtoui64( mData.c_str(), 0, 0 );
	}
	operator DWORD() const
	{
		return strtoul( mData.c_str(), 0, 0 );
	}
	operator float() const
	{
		return (float)atof( mData.c_str() );
	}
	operator double() const
	{
		return strtod( mData.c_str(), 0 );
	}
	operator const char*( ) const
	{
		return mData.c_str();
	}
	operator bool() const
	{
		return ( 0 == mData.compare( "true" ) || 0 == mData.compare( "1" ) );
	}

	// some interface
	void reset()
	{
		mData.clear();
	}
	bool empty() const
	{
		return mData.empty();
	}
	// data
	const std::string& str() const
	{
		return mData;
	}
	const char* str_c() const
	{
		return mData.c_str();
	}

private:
	std::string		mData;
};
class CTokenString
{
public:
	CTokenString( char cDelimiter )
		: m_cDelimiter( cDelimiter )
	{
	}
	CTokenString( const char* strContent, char cDelimiter, bool bRemoveNil = false )
		: m_cDelimiter( cDelimiter )
	{
		SetContent( strContent, bRemoveNil );
	}
	void SetContent( const char* strContent, bool bRemoveNil )
	{
		if( nullptr == strContent )
			return;
		m_vecTokens.clear();
		int nBeginPos = 0;
		int pos = 0;
		while( true )
		{
			while( true )
			{
				if( strContent[pos] == m_cDelimiter || 0 == strContent[pos] )
					break;
				++pos;
			}
			if( !bRemoveNil || pos != nBeginPos )
				m_vecTokens.push_back( CAnyString( std::string( &strContent[nBeginPos], pos - nBeginPos ) ) );
			if( 0 == strContent[pos] )
				break;
			++pos;
			nBeginPos = pos;
		}
	}
	size_t size() const
	{
		return m_vecTokens.size();
	}
	const CAnyString& operator[]( size_t nIndex ) const
	{
		if( nIndex >= m_vecTokens.size() )
		{
			static CAnyString strSomeString;
			return strSomeString;
		}
		return m_vecTokens[nIndex];
	}
private:
	char	m_cDelimiter;
	std::vector<CAnyString> m_vecTokens;
};

void sleep( int32_t _time )
{
#if defined( _WINDOWS )
	Sleep( 1 );
#endif
}

bool GetSending( std::string& _value )
{
	std::lock_guard<std::mutex> lock( g_list_mutex );
	if( g_sending_list.empty() )
		return false;
	_value = g_sending_list.front();
	g_sending_list.pop();
	return true;
}

#define MAX_CHAT_CONTENT 102400

void OnRecvMessage( netstream_t _netstream, const NetStreamPacket _packet )
{
	struct ChatMessage
	{
		uint32_t user_id;
		char content[MAX_CHAT_CONTENT];
		uint32_t size() const
		{
			return (uint32_t)( sizeof( ChatMessage )-( MAX_CHAT_CONTENT - ( strlen( content ) + 1 ) ) );
		}
	};
	ChatMessage message;
	message.user_id = _packet.net_conn_id;
	strncpy( message.content, (const char*)_packet.data, MAX_CHAT_CONTENT );
	std::for_each( all_connections.begin(), all_connections.end(),
		[&]( const NetConnId _conn_id )
	{
		netstream_send( _netstream, _conn_id, &message, message.size() );
	}
	);
}

void thread_func()
{
	printf( "enter thread!\n" );
	netstream_t netstream = netstream_create( nullptr );
	NetPeerId peer_id = netstream_listen( netstream, "", 7234 );
	if( 0 == peer_id )
		return;
	while( g_running )
	{
		std::string value;
		while( GetSending( value ) )
		{
			struct ChatMessage
			{
				uint32_t user_id;
				char content[MAX_CHAT_CONTENT];
				uint32_t size() const
				{
					return sizeof( ChatMessage ) - ( MAX_CHAT_CONTENT - ( strlen( content ) + 1 ) );
				}
			};
			ChatMessage message;
			message.user_id = 0;
			strncpy( message.content, value.c_str(), MAX_CHAT_CONTENT - 1 );
			message.content[MAX_CHAT_CONTENT - 1] = 0;
			std::for_each( all_connections.begin(), all_connections.end(),
				[&]( const NetConnId _conn_id )
			{
				netstream_send( netstream, _conn_id, &message, message.size() );
			}
			);
		}
		NetStreamPacket packet;
		if( 0 != netstream_recv( netstream, packet ) )
		{
			sleep( 1 );
			continue;
		}
		switch( packet.packet_type )
		{
		case MESSAGE_TYPE_CONNECTED:
			all_connections.push_back( packet.net_conn_id );
			printf( "A new connection[%u] has arrived.\n", packet.net_conn_id );
			break;
		case MESSAGE_TYPE_DISCONNECTED:
			all_connections.remove_if(
				[&packet]( const NetConnId& _conn_id )
			{
				return _conn_id == packet.net_conn_id;
			}
			);
			printf( "A connection[%u] has lost.\n", packet.net_conn_id );
			break;
		case MESSAGE_TYPE_MESSAGE:
			OnRecvMessage( netstream, packet );
			break;
		}
		netstream_free_packet( &packet );
	}
	netstream_close( netstream, peer_id );
	netstream_destroy( netstream );
	printf( "exit thread!\n" );
}


int main( int32_t argc, char* argv[] )
{
	std::thread m( thread_func );
	char line_buffer[MAX_CHAT_CONTENT];
	while( true )
	{
		std::cin.getline( line_buffer, MAX_CHAT_CONTENT );
		printf( "CMD: %s\n", line_buffer );
		CTokenString tokens( line_buffer, ' ', true );
		if( 0 == strcmp( tokens[0], "exit" ) )
			break;
		if( 0 == strcmp( tokens[0], "pack" ) )
		{
			std::string assemble_string( tokens[2], tokens[1][0] );
			std::lock_guard<std::mutex> lock( g_list_mutex );
			g_sending_list.push( assemble_string );
		}
		else
		{
			std::lock_guard<std::mutex> lock( g_list_mutex );
			g_sending_list.push( line_buffer );
		}
	}
	g_running = false;
	m.join();
}
