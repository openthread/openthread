#
#  compiling/installing sources for COSE-C
#

if (use_embedtls)
  set (cose_crypto mbedtls.c)
else ()
  set (cose_crypto openssl.c)
endif()

set ( cose_sources 
  Cose.c
  key.c
	MacMessage.c
        MacMessage0.c
	Sign.c
        Sign0.c
	cbor.c
	Encrypt.c
        Encrypt0.c
	Recipient.c
	SignerInfo.c
        ${cose_crypto}
)


if (use_context)
    add_definitions(-DUSE_CBOR_CONTEXT)
endif()

add_library ( cose-c ${LIB_TYPE} ${cose_sources} )

target_include_directories ( cose-c PRIVATE ${OPENSSL_INCLUDE_DIR} )
target_include_directories ( cose-c PUBLIC ${dist_dir}/include )
target_include_directories ( cose-c PUBLIC ../include )
target_include_directories ( cose-c PRIVATE ../src )

target_link_libraries ( cose-c PRIVATE ${OPENSSL_LIBRARIES} )
target_link_libraries ( cose-c PRIVATE cn-cbor )
if (use_embedtls)
    target_include_directories ( cose-c PUBLIC ${CMAKE_SHARED_MODLE_PREFIX}embedtls${CMAKE_SHARED_LIBRARY_SUFFIX}/include )
    target_link_libraries ( cose-c PRIVATE embedtls )
endif()

if ( MSVC )
    target_link_libraries( cose-c PRIVATE ws2_32 )
endif ()

install ( TARGETS cose-c
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	RUNTIME DESTINATION bin)

set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)
if (coveralls)
    include(Coveralls)
    coveralls_turn_on_coverage()

    set(COVERAGE_SRC "")
    foreach (S ${cose_sources})
       get_filename_component(S_ABS ${S} ABSOLUTE)
       list (APPEND COVERAGE_SRCS ${S_ABS})
    endforeach()

    # Create the coveralls target.
    coveralls_setup(
    	"${COVERAGE_SRCS}"
  	${coveralls_send}	# If we should upload
    )
endif()
