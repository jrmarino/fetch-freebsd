# static, shared, and PIC fetch library


set (incssl ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_INSTALL_PREFIX}/libressl/include)

include_directories(
	${CMAKE_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	${incssl}
)

set (libsrcs
	${CMAKE_SOURCE_DIR}/fetch.c 
	${CMAKE_SOURCE_DIR}/common.c 
	${CMAKE_SOURCE_DIR}/ftp.c 
	${CMAKE_SOURCE_DIR}/http.c 
	${CMAKE_SOURCE_DIR}/file.c
	${CMAKE_SOURCE_DIR}/estream.c
	${CMAKE_SOURCE_DIR}/estream-printf.c
)

set (genheaders
	${CMAKE_CURRENT_BINARY_DIR}/httperr.h
	${CMAKE_CURRENT_BINARY_DIR}/ftperr.h
)

add_custom_command(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/httperr.h
	DEPENDS ${CMAKE_SOURCE_DIR}/http.errors
	COMMAND /bin/sh ${CMAKE_SOURCE_DIR}/generate_errors.sh http "${CMAKE_CURRENT_BINARY_DIR}"
)
add_custom_command(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ftperr.h
	DEPENDS ${CMAKE_SOURCE_DIR}/ftp.errors
	COMMAND /bin/sh ${CMAKE_SOURCE_DIR}/generate_errors.sh ftp "${CMAKE_CURRENT_BINARY_DIR}"
)

set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/httperr.h PROPERTIES GENERATED true)
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/ftperr.h PROPERTIES GENERATED true)

add_library(fetchobj ${libsrcs} ${genheaders})
set_property(TARGET fetchobj PROPERTY POSITION_INDEPENDENT_CODE 1)

add_library(fetch SHARED $<TARGET_OBJECTS:fetchobj>)
add_library(fetch_pic STATIC $<TARGET_OBJECTS:fetchobj>)
add_library(fetch_static ${libsrcs} ${genheaders})

set_target_properties(fetch PROPERTIES VERSION 1.0)
set_target_properties(fetch PROPERTIES SOVERSION 1)

target_compile_options(fetchobj PUBLIC -Wno-psabi)
target_compile_options(fetch_static PUBLIC -Wno-psabi)

install(TARGETS fetch
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(TARGETS fetch_pic
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libfetch_static.a
	RENAME libfetch.a
	DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(FILES ${CMAKE_SOURCE_DIR}/fetch-estream.h
	${CMAKE_SOURCE_DIR}/fetch.h
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(FILES ${CMAKE_SOURCE_DIR}/fetch.3 DESTINATION ${CMAKE_INSTALL_MANDIR}/man3)
