#
#
# top level build file for cn-cbor

## prepare CMAKE
cmake_minimum_required ( VERSION 3.0.0 )

set ( VERSION_MAJOR 0   CACHE STRING "Project major version number")
set ( VERSION_MINOR "1" CACHE STRING "Project minor version number" )
set ( VERSION_PATCH "0" CACHE STRING "Project patch version number" )
set ( CN_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}" )
mark_as_advanced(VERSION_MAJOR VERSION_MINOR VERSION_PATCH CN_VERSION)

project ( "cn-cbor" VERSION "${CN_VERSION}")

find_package(Doxygen)

## setup options
option ( use_context    "Use context pointer for CBOR functions" OFF )
option ( verbose        "Produce verbose makefile output" OFF )
option ( optimize       "Optimize for size" OFF )
option ( fatal_warnings "Treat build warnings as errors" ON )
option ( coveralls      "Generate coveralls data" ON )
option ( coveralls_send "Send data to coveralls site" OFF )
option ( build_docs "Create docs using Doxygen" ${DOXYGEN_FOUND} )
option ( build_shared_libs "Build Shared Libraries" ON)
option ( no_floats "Build without floating point support" OFF )

set ( dist_dir    ${CMAKE_BINARY_DIR}/dist )
set ( prefix      ${CMAKE_INSTALL_PREFIX} )
set ( exec_prefix ${CMAKE_INSTALL_PREFIX}/bin )
set ( libdir      ${CMAKE_INSTALL_PREFIX}/lib )
set ( includedir  ${CMAKE_INSTALL_PREFIX}/include )
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cn-cbor.pc.in
               ${CMAKE_CURRENT_BINARY_DIR}/cn-cbor.pc @ONLY)
install (FILES ${CMAKE_CURRENT_BINARY_DIR}/cn-cbor.pc DESTINATION lib/pkgconfig )

set ( package_prefix "${CMAKE_PACKAGE_NAME}-${CMAKE_SYSTEM_NAME}" )

set ( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${dist_dir}/bin )
set ( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${dist_dir}/lib )
set ( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${dist_dir}/lib )

if (NOT CMAKE_BUILD_TYPE)
  if ( optimize )
    set ( CMAKE_BUILD_TYPE MinSizeRel )
    set ( coveralls OFF )
    set ( coveralls_send OFF )
  else ()
    set ( CMAKE_BUILD_TYPE Debug )
  endif ()
endif()

if ( MSVC )
   set (coveralls OFF)
endif ()

set (LIB_TYPE STATIC)
if (build_shared_libs)
  set (LIB_TYPE SHARED)
endif (build_shared_libs)

message ( "Build type: ${CMAKE_BUILD_TYPE}" )

if ( CMAKE_C_COMPILER_ID STREQUAL "GNU" OR
     CMAKE_C_COMPILER_ID MATCHES "Clang" )
  message ( STATUS "adding GCC/Clang options ")
  add_definitions ( -std=gnu99 -Wall -Wextra -pedantic )
  if ( fatal_warnings )
    add_definitions ( -Werror )
  endif ()
  if ( optimize )
    add_definitions ( -Os )
  endif ()
elseif ( MSVC )
  add_definitions ( /W3 )
  if ( fatal_warnings )
    add_definitions ( /WX )
  endif ()
else ()
  message ( FATAL_ERROR "unhandled compiler id: ${CMAKE_C_COMPILER_ID}" )
endif ()

if ( no_floats )
   add_definitions(-DCBOR_NO_FLOAT)
endif()

if ( verbose )
  set ( CMAKE_VERBOSE_MAKEFILE ON )
endif ()

## include the parts
add_subdirectory ( include )
add_subdirectory ( src )
add_subdirectory ( test )

install (FILES LICENSE README.md DESTINATION .)

## setup packaging
set ( CPACK_GENERATOR "TGZ" )
set ( CPACK_PACKAGE_VERSION "${PROJECT_VERSION}" )
set ( CPACK_SOURCE_GENERATOR "TGZ" )
set ( CPACK_SOURCE_IGNORE_FILES "/\\\\.git/" )
file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/.gitignore igs)
foreach (ig IN ITEMS ${igs})
    # remove comments
    string ( REGEX REPLACE "^\\s*#.*" "" ig "${ig}")
    # remove any other whitespace
    string ( STRIP "${ig}" ig)
    # anything left?
    if (ig)
      # dots are literal
      string ( REPLACE "." "\\\\." ig "${ig}" )
      # stars are on thars
      string ( REPLACE "*" ".*" ig "${ig}" )
      list ( APPEND CPACK_SOURCE_IGNORE_FILES "/${ig}/" )
    endif()
endforeach()

set ( CPACK_PACKAGE_DESCRIPTION_FILE ${CMAKE_CURRENT_SOURCE_DIR}/README.md )
set ( CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE" )

include ( CPack )
include ( CTest )

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)
include ( LCov )

if (build_docs)
    if(NOT DOXYGEN_FOUND)
        message(FATAL_ERROR "Doxygen is needed to build the documentation.")
    endif()

    set(doxyfile_in ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
    set(doxyfile ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

    configure_file(${doxyfile_in} ${doxyfile} @ONLY)

    add_custom_target(doc
        COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM)

    install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html DESTINATION share/doc)
endif()
