#include <stdint.h>
#include <thread>
#include <iostream>
#include <queue>
#include <string>
#include <mutex>
#include <string.h>
#include <cinttypes>
#include "libnetstream.h"

#if defined( _WINDOWS )
#include <windows.h>
#endif

#define MAX_CHAT_CONTENT 1024

bool g_running = true;
bool g_close_connection = false;
std::string g_address;
int32_t g_port;
std::mutex g_list_mutex;
std::queue<std::string> g_sending_list;

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

void OnRecvMessage( const NetStreamPacket _packet )
{
	struct ChatMessage
	{
		uint64_t user_id;
		char content[MAX_CHAT_CONTENT];
	};
	ChatMessage* message = (ChatMessage*)_packet.data;
	//printf( "[%u] said: %s\n", message->user_id, message->content );
	printf( "[%" PRIu64",%" PRIu64",%" PRIu32"] say : %s\n", message->user_id, strlen( message->content ), _packet.data_size, message->content );
}

void thread_func()
{
	printf( "enter thread!\n" );
	netstream_t netstream = netstream_create( nullptr, nullptr, 0 );
	NetPeerId peer_id = netstream_connect( netstream, g_address.c_str(), g_port );
	if( 0 == peer_id )
		return;
	NetConnId conn_id = 0;
	while( g_running )
	{
		if( g_close_connection )
		{
			netstream_disconnect( netstream, conn_id );
			g_close_connection = false;
		}
		// send
		std::string value;
		while( GetSending( value ) )
		{
			netstream_send( netstream, conn_id, value.c_str(), (uint32_t)value.size() + 1 );
		}
		// recv
		NetStreamPacket packet;
		if( 0 != netstream_recv( netstream, packet ) )
		{
			sleep( 1 );
			continue;
		}
		switch( packet.packet_type )
		{
		case MESSAGE_TYPE_CONNECTED:
			conn_id = packet.net_conn_id;
			printf( "Connected to server %s %" PRId32", I'm [%" PRIu64"]\n"
				, netstream_remote_ip( netstream, conn_id )
				, netstream_remote_port( netstream, conn_id )
				, conn_id );
			break;
		case MESSAGE_TYPE_DISCONNECTED:
			printf( "Disconnected from server %s %" PRId32", I'm [%" PRIu64"]\n"
				, netstream_remote_ip( netstream, conn_id )
				, netstream_remote_port( netstream, conn_id )
				, conn_id );
			conn_id = 0;
			break;
		case MESSAGE_TYPE_MESSAGE:
			OnRecvMessage( packet );
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
	if( argc != 3 )
	{
		printf( "%s <ipv4 or ipv6 address> <remote port>\n", argv[0] );
		return -1;
	}
	g_address = argv[1];
	g_port = atoi( argv[2] );
	std::thread m( thread_func );
	char line_buffer[MAX_CHAT_CONTENT];
	while( true )
	{
		std::cin.getline( line_buffer, MAX_CHAT_CONTENT );
		printf( "CMD: %s\n", line_buffer );
		if( 0 == strcmp( line_buffer, "exit" ) )
			break;
		if( 0 == strcmp( line_buffer, "close" ) )
		{
			g_close_connection = true;
			continue;
		}
		std::lock_guard<std::mutex> lock( g_list_mutex );
		g_sending_list.push( line_buffer );
	}
	g_running = false;
	m.join();
}
