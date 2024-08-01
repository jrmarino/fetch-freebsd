set (prog fetchx)

if (USE_SYSTEM_SSL)
	set (libssl ssl)
	set (libcrypto crypto)
	set (incssl "")
else()
	set (libssl ${CMAKE_INSTALL_PREFIX}/libressl/lib/libssl_pic.a)
	set (libcrypto ${CMAKE_INSTALL_PREFIX}/libressl/lib/libcrypto_pic.a)
	set (incssl ${CMAKE_CURRENT_SOURCE_DIR}
		    ${CMAKE_INSTALL_PREFIX}/libressl/include)
endif()

set (libs ${libssl} ${libcrypto})

if (CMAKE_SYSTEM_NAME MATCHES "(Solaris|SunOS)")
   set (libs ${libs} resolv socket nsl)
endif()

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
   set (libs ${libs} pthread)
endif()

set (srcs
	${CMAKE_SOURCE_DIR}/main.c
	${CMAKE_SOURCE_DIR}/fetch.c 
	${CMAKE_SOURCE_DIR}/common.c 
	${CMAKE_SOURCE_DIR}/ftp.c 
	${CMAKE_SOURCE_DIR}/http.c 
	${CMAKE_SOURCE_DIR}/file.c
	${CMAKE_SOURCE_DIR}/estream.c
	${CMAKE_SOURCE_DIR}/estream-printf.c
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

add_executable(${prog} ${srcs})
target_link_libraries(${prog} ${libs})
target_include_directories(${prog} PUBLIC
	${incssl}
	${CMAKE_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
)
target_compile_options(${prog} PUBLIC -Wno-psabi)

# install(TARGETS ${prog} DESTINATION ${CMAKE_INSTALL_BINDIR})
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${prog} DESTINATION ${CMAKE_INSTALL_BINDIR} RENAME fetch)
install(FILES ${CMAKE_SOURCE_DIR}/fetch.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
