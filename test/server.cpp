#include <stdint.h>
#include <conio.h>
#include <list>
#include <iostream>
#include <algorithm>
#include <thread>
#include "libnetstream.h"

#if defined( _WINDOWS )
#include <windows.h>
#endif

bool g_running = true;
std::list<NetConnId> all_connections;

void sleep( int32_t _time )
{
#if defined( _WINDOWS )
	Sleep( 1 );
#endif
}

#define MAX_CHAT_CONTENT 1024

void OnRecvMessage( netstream_t _netstream, const NetStreamPacket _packet )
{
	struct ChatMessage
	{
		uint32_t user_id;
		char content[MAX_CHAT_CONTENT];
		uint32_t size() const
		{
			return sizeof( ChatMessage )-( MAX_CHAT_CONTENT - ( strlen( content ) + 1 ) );
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
	netstream_t netstream = netstream_create();
	NetPeerId peer_id = netstream_listen( netstream, "", 7234 );
	if( 0 == peer_id )
		return;
	while( g_running )
	{
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
		if( 0 == strcmp( line_buffer, "exit" ) )
			break;
	}
	g_running = false;
	m.join();
}
