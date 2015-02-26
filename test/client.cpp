#include <stdint.h>
#include <thread>
#include <iostream>
#include <queue>
#include <string>
#include <mutex>
#include "libnetstream.h"

#if defined( _WINDOWS )
#include <windows.h>
#endif

#define MAX_CHAT_CONTENT 1024

bool g_running = true;
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
		uint32_t user_id;
		char content[MAX_CHAT_CONTENT];
	};
	ChatMessage* message = (ChatMessage*)_packet.data;
	//printf( "[%u] said: %s\n", message->user_id, message->content );
	printf( "[%u] said something: %d/%d\n", message->user_id, strlen( message->content ), _packet.data_size );
}

void thread_func()
{
	printf( "enter thread!\n" );
	netstream_t netstream = netstream_create( nullptr );
	NetPeerId peer_id = netstream_connect( netstream, "127.0.0.1", 7234 );
	if( 0 == peer_id )
		return;
	NetConnId conn_id = 0;
	while( g_running )
	{
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
			printf( "Connected to server, I'm [%u]\n", conn_id );
			break;
		case MESSAGE_TYPE_DISCONNECTED:
			printf( "Disconnected from server, I'm [%u]\n", conn_id );
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
	std::thread m( thread_func );
	char line_buffer[MAX_CHAT_CONTENT];
	while( true )
	{
		std::cin.getline( line_buffer, MAX_CHAT_CONTENT );
		printf( "CMD: %s\n", line_buffer );
		if( 0 == strcmp( line_buffer, "exit" ) )
			break;
		std::lock_guard<std::mutex> lock( g_list_mutex );
		g_sending_list.push( line_buffer );
	}
	g_running = false;
	m.join();
}
