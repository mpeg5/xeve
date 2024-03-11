cmake_minimum_required (VERSION 3.12)

function(get_versions versionstring VERSION_MAJOR VERSION_MINOR VERSION_PATCH)
    string(REGEX REPLACE "^([vV])([0-9]*)([.][0-9]*[.][0-9]*-?.*)$" "\\2" numbers ${versionstring} )
    set(VERSION_MAJOR ${numbers} PARENT_SCOPE)
    string(REGEX REPLACE "^([vV][0-9]*[.])([0-9]*)([.][0-9]*-?.*)$" "\\2" numbers ${versionstring} )
    set(VERSION_MINOR ${numbers} PARENT_SCOPE)
    string(REGEX REPLACE "^([vV][0-9]*[.][0-9]*[.])([0-9]*)(-?.*)$" "\\2" numbers ${versionstring} )
    set(VERSION_PATCH ${numbers} PARENT_SCOPE)
endfunction()

execute_process(
    COMMAND git describe --match "v[0-9]*.[0-9]*.[0-9]*" --abbrev=0 --tags
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    RESULT_VARIABLE RESULT
    OUTPUT_VARIABLE OUTPUT)

if(NOT RESULT EQUAL 0)

  find_program(GIT_COMMAND "git")
  if(GIT_COMMAND)
    message(WARNING "There is no valid git repository in your working directory \"${CMAKE_SOURCE_DIR}\" .")
  else()
    message(WARNING "You have no git tool installed.")
  endif()

  if(NOT EXISTS "${CMAKE_SOURCE_DIR}/version.txt")
    message(FATAL_ERROR
	    "version.txt file doesn't exist!\n"
	    "Since your working directory doesn't contain a git repository you must provide \"${CMAKE_SOURCE_DIR}/version.txt\" file containing a valid version string.\n"
	    "The version string provided to version.txt must match the following format:\n\tv[VERSION_MAJOR].[VERSION_MINOR].[VERSION_PATCH]\n"
	    "To get the information on version of the downloaded library please follow the link below:\n\t https://github.com/mpeg5/xeve"
	    )
  endif()

  message("Version string has been taken from version.txt file.")
  file(STRINGS "version.txt" VERSION_STRING)

else()
  message("Version string has been taken from git tag.")
  set(VERSION_STRING ${OUTPUT})
  string(REGEX REPLACE "\n$" "" VERSION_STRING "${VERSION_STRING}")
endif()

if(NOT ${VERSION_STRING} MATCHES "^([vV])([0-9]*)([.][0-9]*[.][0-9]*-?.*)$")
  message(FATAL_ERROR "Version string ${VERSION_STRING} doesn't match required format v[VERSION_MAJOR].[VERSION_MINOR].[VERSION_PATCH]")
endif()

get_versions(${VERSION_STRING} VERSION_MAJOR VERSION_MINOR VERSION_PATCH)
if(VERSION_MAJOR STREQUAL ${VERSION_STRING})
  message(FATAL_ERROR "Version string parsing error")
endif()
message("XEVE VERSION=${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

add_definitions(-Dxeve_dynamic_EXPORTS)

# Check input arguments.
if(NOT SET_PROF)
  set(SET_PROF "MAIN")
endif()

if(NOT ARM)
  set(ARM "FALSE")
else()
  add_definitions(-DARM=1)
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -flax-vector-conversions")
  set(CMAKE_EXE_LINKER_FLAGS "-static")
endif()
message("ARM=${ARM}")

if(("${SET_PROF}" STREQUAL "MAIN") OR ("${SET_PROF}" STREQUAL "BASE"))
  message("SET_PROF=${SET_PROF}")
else()
  message("SET_PORF should be BASE or MAIN [Input error: ${SET_PROF}]")
  return()
endif()

cmake_policy(SET CMP0048 NEW)
set(CMAKE_C_STANDARD 99)

# Maps to a solution file (XEVE.sln).
project (XEVE VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH} LANGUAGES C)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Set compiler flags and options.
if( MSVC )
    add_compile_options(
        $<$<CONFIG:>:/MD> #---------|
        $<$<CONFIG:Debug>:/MDd> #---|-- Dynamically link the runtime libraries
        $<$<CONFIG:Release>:/MD> #--|
    )
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
message ("c Flags: " ${CMAKE_C_FLAGS})
message ("linker Flags: " ${CMAKE_EXE_LINKER_FLAGS})

# Sub-directories where more CMakeLists.txt exist
set( LIB_NAME xeve)
set( LIB_NAME_BASE xeveb)

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

########################################
# add CPack to project
########################################

if( MSVC )
  set(CPACK_GENERATOR "NSIS")
elseif( UNIX )
  set(CPACK_GENERATOR "DEB")
endif()

# Package name
if(("${SET_PROF}" STREQUAL "BASE"))
  set(PACKAGE_PROFILE_NAME "base")
endif()

if(("${SET_PROF}" STREQUAL "MAIN"))
  set(PACKAGE_PROFILE_NAME "main")
endif()

string(TOLOWER "${PROJECT_NAME}" PACKAGE_NAME)
string(CONCAT PACKAGE_NAME ${PACKAGE_NAME} "-${PACKAGE_PROFILE_NAME}")

set(CPACK_PACKAGE_NAME ${PACKAGE_NAME})

# Package License & Readme file
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/COPYING")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")

# Package checksum
set(CPACK_PACKAGE_CHECKSUM MD5)

# Short description of the project
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
"eXtra-fast Essential Video Encoder, MPEG-5 EVC (Essential Video Coding), ${PACKAGE_PROFILE_NAME} profile")

# Project homepage
set(CPACK_PACKAGE_VENDOR "MPEG-5")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/mpeg5/xeve/releases")
set(CMAKE_PROJECT_HOMEPAGE_URL "https://github.com/mpeg5/xeve")

# Installation directory on the target system
string(TOLOWER "${PROJECT_NAME}" PROJECT_NAME_LOWER)
set(PACKAGE_INSTALL_DIRECTORY "${PROJECT_NAME_LOWER} ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${PACKAGE_INSTALL_DIRECTORY}")

if( UNIX )
  set(CPACK_DEBIAN_PACKAGE_MAINTAINER "MPEG-Five") #required​
  set(CPACK_PACKAGE_CONTACT "mpegfive@gmail.com") #required

  set(CPACK_DEB_COMPONENT_INSTALL ON)

  # If enabled (ON) multiple packages are generated.
  # By default a single package containing files of all components is generated.
  set(CPACK_DEB_COMPONENT_INSTALL YES)

  # This may be set to DEB-DEFAULT to allow CPackDeb to generate package file name by itself in deb format:
  # <PackageName>_<VersionNumber>-<DebianRevisionNumber>_<DebianArchitecture>.deb
  set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

  set(CPACK_DEBIAN_RUNTIMEGROUP_PACKAGE_NAME "${CPACK_PACKAGE_NAME}")
  set(CPACK_DEBIAN_DEVELOPMENTGROUP_PACKAGE_NAME "${CPACK_PACKAGE_NAME}-dev")
  set(CPACK_DEBIAN_DEVELOPMENTGROUP_PACKAGE_DEPENDS ${CPACK_PACKAGE_NAME})
endif()

set(CPACK_COMPONENTS_ALL Runtime Libraries Development)
set(CPACK_COMPONENT_DEVELOPMENT_DEPENDS Libraries)
set(CPACK_COMPONENT_RUNTIME_DEPENDS Libraries)

include(CPack)

if( MSVC )
  cpack_add_install_type(Full DISPLAY_NAME "All")
  cpack_add_install_type(Developer DISPLAY_NAME "Developer files")
endif()

cpack_add_component_group(DevelopmentGroup
  DISPLAY_NAME "Development"
  EXPANDED
  DESCRIPTION
  "All of the tools you'll ever need to develop libxeve dependent software")

cpack_add_component_group(RuntimeGroup
  DISPLAY_NAME "Runtime"
  EXPANDED
  DESCRIPTION
 "Shared library and xeve_app reference application")

cpack_add_component(Runtime DISPLAY_NAME "Application" DESCRIPTION  "Reference application that makes use of libxeve" GROUP RuntimeGroup INSTALL_TYPES Full Runtime)
cpack_add_component(Libraries DISPLAY_NAME "Shared Libraries" DESCRIPTION "Shared libraries used to run xeve_app" GROUP RuntimeGroup INSTALL_TYPES Full Runtime Developer)
cpack_add_component(Development DISPLAY_NAME "C++ Headers and static libraries used to build programs with libxeve" DESCRIPTION "C/C++ header files for use with libxeve" GROUP DevelopmentGroup INSTALL_TYPES Full Developer)

