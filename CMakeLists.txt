cmake_minimum_required (VERSION 3.12)

# Check input arguments.
if(NOT SET_PROF)
  set(SET_PROF "MAIN")
endif()

if(("${SET_PROF}" STREQUAL "MAIN") OR ("${SET_PROF}" STREQUAL "BASE"))
  message("SET_PROF=${SET_PROF}")
else()
  message("SET_PORF should be BASE or MAIN [Input error: ${SET_PROF}]")
  return()
endif()

option(XEVE_APP_STATIC_BUILD "xeve_app will be statically linked against static xeve library" ON)
if(XEVE_APP_STATIC_BUILD)
    add_definitions(-DXEVE_STATIC_DEFINE)
endif(XEVE_APP_STATIC_BUILD)

cmake_policy(SET CMP0048 NEW)
set(CMAKE_C_STANDARD 99)

# Maps to a solution file (XEVE.sln).
project (XEVE VERSION 1.0.0)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Set compiler flags and options. 

message("Processor byte order: ${CMAKE_C_BYTE_ORDER}")

if( ${CMAKE_C_BYTE_ORDER} STREQUAL "LITTLE_ENDIAN")
  set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} -DLITTLE_ENDIAN_BYTE_ORDER)
else()
  set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} -DBIG_ENDIAN_BYTE_ORDER)
endif()

if( MSVC )
elseif( UNIX OR MINGW )
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "Release")
    endif()
    message("CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")

    if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
        set(OPT_LV "O0")
        set(OPT_DBG "-g")
    else()
        set(OPT_LV "O3")
        set(OPT_DBG "-DNDEBUG") # disable assert 
    endif()

    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OPT_DBG} -${OPT_LV} -fomit-frame-pointer -Wall -Wno-unused-function -Wno-unused-but-set-variable -Wno-unused-variable -Wno-attributes -Werror -Wno-strict-overflow -Wno-unknown-pragmas -Wno-stringop-overflow -std=c99")
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-pointer-sign -pthread -Wno-pointer-to-int-cast -Wno-maybe-uninitialized")
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lm")
endif()

# Command to output information to the console
message ("C Flags: " ${CMAKE_C_FLAGS})
message ("Linker Flags: " ${CMAKE_EXE_LINKER_FLAGS})

# Sub-directories where more CMakeLists.txt exist
if(("${SET_PROF}" STREQUAL "BASE"))
   add_subdirectory(src_base)
endif()
if(("${SET_PROF}" STREQUAL "MAIN"))
   add_subdirectory(src_main)
endif()
add_subdirectory(app)

# uninstall target
if(NOT TARGET uninstall)
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

  add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
endif()
