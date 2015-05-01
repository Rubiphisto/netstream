#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#if defined _WIN32 || defined __CYGWIN__
#	ifdef LIBNETSTREAM_DLL
#		ifdef LIBNETSTREAM_EXPORT
#			define LIBNETSTREAM_API __declspec(dllexport)
#		else
#			define LIBNETSTREAM_API __declspec(dllimport)
#		endif
#	else // LIBNETSTREAM_DLL
#		define LIBNETSTREAM_API
#	endif // LIBNETSTREAM_DLL
#else
#	if __GUNC__ >= 4
#		define LIBNETSTREAM_API __attribute__ ((visibility("default")))
#	else
#		define LIBNETSTREAM_API
#	endif
#endif

struct netstream;
typedef struct netstream* netstream_t;

typedef uint32_t NetConnId;
typedef uint16_t NetPeerId;

const uint8_t MESSAGE_TYPE_CONNECTED = 0;
const uint8_t MESSAGE_TYPE_MESSAGE = 1;
const uint8_t MESSAGE_TYPE_DISCONNECTED = 2;

struct NetStreamPacket
{
	uint8_t		packet_type;
	NetPeerId	peer_id;
	NetConnId	net_conn_id;
	uint32_t	data_size;
	uint8_t*	data;
};

typedef void( *PacketArrivedHandler )( netstream_t, const NetStreamPacket&, uint64_t );

LIBNETSTREAM_API netstream_t netstream_create( PacketArrivedHandler _handler, uint64_t _param );
LIBNETSTREAM_API NetPeerId netstream_listen( netstream_t _net_stream, const char* _local_addr, uint16_t _port );
LIBNETSTREAM_API NetPeerId netstream_connect( netstream_t _net_stream, const char* _remote_addr, uint16_t _port );
LIBNETSTREAM_API bool netstream_disconnect( netstream_t _net_stream, NetConnId _conn_id );
LIBNETSTREAM_API bool netstream_close( netstream_t _net_stream, NetPeerId _peer_id );
LIBNETSTREAM_API int32_t netstream_recv( netstream_t _net_stream, NetStreamPacket& _packet );
LIBNETSTREAM_API int32_t netstream_send( netstream_t _net_stream, NetConnId _conn_id, const void* _data, uint32_t _size );
LIBNETSTREAM_API void netstream_free_packet( NetStreamPacket* _packet );
LIBNETSTREAM_API void netstream_destroy( netstream_t _net_stream );

#ifdef __cplusplus
}
#endif // __cplusplus
