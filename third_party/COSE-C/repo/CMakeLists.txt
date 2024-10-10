#
#  top level build file for COSE-C
#

## prepare CMAKE
cmake_minimum_required( VERSION 3.0.0)

set (VERSION_MAJOR 0 CACHE STRING "Project major version number")
set (VERSION_MINOR "1" CACHE STRING "Project minor version number")
set (VERSION_PATCH "0" CACHE STRING "Project patch version number")
set (COSE_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
mark_as_advanced(VERSION_MAJOR VERSION_MINOR VERSION_PATCH COSE_VERSION)

project ("cose-c" VERSION "${COSE_VERSION}")

find_package(Doxygen)
find_package(OpenSSL 1.0 REQUIRED)

### setup options
option (use_context	"Use context pointer for COSE functions" ON)
option (verbose         "Produce verbose makefile output" OFF)
option (optimize        "Optimize for size" OFF)
option (fatal_warnings  "Treat build warnings as error" ON)
option (coveralls       "Generate coveralls data" ON)
option ( coveralls_send "Send data to coveralls site" OFF )
option (build_docs      "Create docs using Doxygen" ${DOXYGEN_FOUND} )
option (build_shared_libs "Build Shared Libraries" ON)
option (use_embedtls    "Use MBedTLS for the Crypto Package" OFF)
option (include_encrypt  "Include COSE_ENCRYPT" ON)
option (include_encrypt0 "Include COSE_ENCRYPT0" ON)
option (include_mac      "Include COSE_MAC" ON)
option (include_mac0     "Include COSE_MAC0" ON)
option (include_sign     "Include COSE_SIGN" ON)
option (include_sign0    "Include COSE_SIGN0" ON)

set ( dist_dir          ${CMAKE_BINARY_DIR}/dist )
set ( prefix            ${CMAKE_INSTALL_PREFIX} )
set ( exec_prefix       ${CMAKE_INSTALL_PREFIX}/bin )
set ( libdir            ${CMAKE_INSTALL_PREFIX}/lib )
set ( includedir        ${CMAKE_INSTALL_PREFIX}/include )


if (NOT CMAKE_BUILD_TYPE)
   if (optimize)
      set ( CMAKE_BUILD_TYPE MinSizeRel )
      set ( coveralls OFF )
      set ( coveralls_send OFF )
   else ()
      set ( CMAKE_BUILD_TYPE Debug)
   endif ()
endif ()

if (NOT include_encrypt)
   add_definitions( -DINCLUDE_ENCRYPT=0 )
endif ()
if (NOT include_encrypt0)
   add_definitions( -DINCLUDE_ENCRYPT0=0 )
endif ()
if (NOT include_mac)
   add_definitions( -DINCLUDE_MAC=0 )
endif ()
if (NOT include_mac0)
   add_definitions( -DINCLUDE_MAC0=0 )
endif ()
if (NOT include_sign)
   add_definitions( -DINCLUDE_SIGN=0 )
endif ()
if (NOT include_sign0)
   add_definitions( -DINCLUDE_SIGN0=0 )
endif ()

if ( MSVC OR CMAKE_C_COMPILER_ID MATCHES "Clang" )
   set (coveralls OFF)
endif ()

message ( "Build type: ${CMAKE_BUILD_TYPE}" )

add_definitions( -DUSE_ARRAY )
if ( CMAKE_C_COMPILER_ID STREQUAL "GNU" OR
     CMAKE_C_COMPILER_ID MATCHES "Clang")
   message ( STATUS "adding GCC/Clang options ")
   add_definitions( -std=gnu99 -Wall -Wextra -pedantic )
   if ( fatal_warnings )
      add_definitions( -Werror )
   endif ()
   if (optimize) 
      add_definitions( -Os )
   endif ()
   add_definitions( -DNDEBUG )
elseif (MSVC)
   add_definitions ( /W4 )
   if (fatal_warnings)
      add_definitions( /WX )
   endif ()
else ()
    message ( FATAL_ERROR "unhandled compiler id: ${CMAKE_C_COMPILER_ID}" )
endif ()

set (LIB_TYPE STATIC)
if (build_shared_libs)
   set (LIB_TYPE SHARED)
endif (build_shared_libs)

if (versbose)
   set (CMAKE_VERBOSE_MAKEFILE ON)
endif ()

include (CTest)

##  try for documentation
if (build_docs)
   if (NOT DOXYGEN_FOUND)
        message(FATAL_ERROR "Doxygen is needed to build the documenation")
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

Include(ExternalProject)
ExternalProject_Add(
  project_cn-cbor
  GIT_REPOSITORY https://github.com/jimsch/cn-cbor
  GIT_TAG complete
  CMAKE_ARGS -Doptimize=OFF -Duse_context=${use_context} -Dbuild_docs=OFF -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR> -Dcoveralls=OFF -Dbuild_shared_libs=${build_shared_libs} -Dfatal_warnings=OFF
  INSTALL_DIR "${dist_dir}"
  UPDATE_DISCONNECTED 1
)

ExternalProject_Get_Property(project_cn-cbor install_dir)
include_directories ( "${install_dir}/include" )

if (MSVC)
   add_library (cn-cbor STATIC IMPORTED)
   set_property (TARGET cn-cbor PROPERTY IMPORTED_LOCATION "${install_dir}/lib/${CMAKE_SHARED_MODULE_PREFIX}cn-cbor.lib")
else ()
   add_library (cn-cbor STATIC IMPORTED)
   set_property (TARGET cn-cbor PROPERTY IMPORTED_LOCATION "${install_dir}/lib/${CMAKE_SHARED_MODULE_PREFIX}cn-cbor${CMAKE_SHARED_LIBRARY_SUFFIX}")
endif ()

add_dependencies(cn-cbor project_cn-cbor)

if (use_embedtls)
   add_definitions( -DUSE_MBED_TLS )
   ExternalProject_Add(
     project_embedtls
     GIT_REPOSITORY https://github.com/ARMmbed/mbedtls
     CMAKE_ARGS -DENABLED_PROGRAMS=OFF -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR> -Dcoveralls=OFF -DUSE_SHARED_MBEDTLS_LIBRARY=${build_shared_libs} -Dfatal_warnings=OFF -DENABLE_TESTING=OFF -DLIB_INSTALL_DIR=${install_dir}/lib
     INTALL_DIR "${dist_dir}"
     UPDATE_DISCONNECTED 1
   )

   ExternalProject_Get_Property(project_embedtls install_dir)
   include_directories( "${install_dir}/include" )
   add_library( embedtls STATIC IMPORTED)
   if (MSVC)
      set_property (TARGET embedtls PROPERTY IMPORTED_LOCATION "${dist_dir}/lib/${CMAKE_SHARED_MODULE_PREFIX}mbedcrypto.lib")
   else ()
      set_property (TARGET embedtls PROPERTY IMPORTED_LOCATION "${dist_dir}/lib/${CMAKE_SHARED_MODULE_PREFIX}mbedcrypto${CMAKE_SHARED_LIBRARY_SUFFIX}")
   endif ()
   add_dependencies(embedtls project_embedtls)
endif ()


## include the parts
add_subdirectory(src)
add_subdirectory(include)
add_subdirectory(test)
add_subdirectory(dumper)

