netstream_library( netstream
	DEFS LIBNETSTREAM_DLL LIBNETSTREAM_EXPORT
	LIBS LibEvent WinSock )

install( TARGETS netstream DESTINATION ${LIB_INSTALL_DIR} )
install( FILES ../include/libnetstream.h DESTINATION ${INCLUDE_INSTALL_DIR} )

