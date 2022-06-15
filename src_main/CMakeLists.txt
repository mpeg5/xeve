# For shared libraries VERSION and SOVERSION can be used to specify the build version and API version respectively
# see https://cmake.org/cmake/help/latest/prop_tgt/SOVERSION.html
#
set( LIB_SOVERSION ${VERSION_MAJOR})

file (GLOB ETM_INC "../inc/*.h")
file (GLOB LIB_BASE_SRC "../src_base/xeve_*.c")
file (GLOB LIB_BASE_INC "../src_base/xeve_*.h" )
file (GLOB LIB_SSE_SRC "../src_base/sse/xeve_*.c")
file (GLOB LIB_SSE_INC "../src_base/sse/xeve_*.h" )
file (GLOB LIB_AVX_SRC "../src_base/avx/xeve_*.c")
file (GLOB LIB_AVX_INC "../src_base/avx/xeve_*.h" )
file (GLOB LIB_NEON_SRC "../src_base/neon/xeve_*.c")
file (GLOB LIB_NEON_INC "../src_base/neon/xeve_*.h" )
file (GLOB LIB_API_MAIN_SRC "./xevem.c")
file (GLOB LIB_MAIN_SRC "./xevem_*.c")
file (GLOB LIB_MAIN_INC "./xevem_*.h" )
file (GLOB LIB_MAIN_SSE_SRC "./sse/xevem_*.c")
file (GLOB LIB_MAIN_SSE_INC "./sse/xevem_*.h" )
file (GLOB LIB_MAIN_AVX_SRC "./avx/xevem_*.c")
file (GLOB LIB_MAIN_AVX_INC "./avx/xevem_*.h" )

include(GenerateExportHeader)
include_directories("${CMAKE_BINARY_DIR}")

if("${ARM}" STREQUAL "TRUE")
  add_library( ${LIB_NAME} STATIC ${LIB_API_MAIN_SRC} ${ETM_INC} ${LIB_BASE_SRC} ${LIB_BASE_INC} ${LIB_MAIN_SRC} ${LIB_MAIN_INC} ${LIB_NEON_INC} ${LIB_NEON_SRC} )
  add_library( ${LIB_NAME}_dynamic SHARED ${LIB_API_MAIN_SRC} ${ETM_INC} ${LIB_BASE_SRC} ${LIB_BASE_INC} ${LIB_MAIN_SRC} ${LIB_MAIN_INC} ${LIB_NEON_INC} ${LIB_NEON_SRC})
else()
  add_library( ${LIB_NAME} STATIC ${LIB_API_MAIN_SRC} ${ETM_INC} ${LIB_BASE_SRC} ${LIB_BASE_INC} ${LIB_MAIN_SRC} ${LIB_MAIN_INC}
                                  ${LIB_SSE_SRC} ${LIB_SSE_INC} ${LIB_MAIN_SSE_SRC} ${LIB_MAIN_SSE_INC} ${LIB_AVX_SRC} ${LIB_AVX_INC} ${LIB_MAIN_AVX_SRC} ${LIB_MAIN_AVX_INC} )
  add_library( ${LIB_NAME}_dynamic SHARED ${LIB_API_MAIN_SRC} ${ETM_INC} ${LIB_BASE_SRC} ${LIB_BASE_INC} ${LIB_MAIN_SRC} ${LIB_MAIN_INC}
                                          ${LIB_SSE_SRC} ${LIB_SSE_INC} ${LIB_MAIN_SSE_SRC} ${LIB_MAIN_SSE_INC} ${LIB_AVX_SRC} ${LIB_AVX_SRC} ${LIB_AVX_INC} ${LIB_MAIN_AVX_SRC} ${LIB_MAIN_AVX_INC})
endif()

set_target_properties(${LIB_NAME}_dynamic PROPERTIES VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR} SOVERSION ${LIB_SOVERSION})

# @todo Consider using WINDOWS_EXPORT_ALL_SYMBOLS instead of generate_export_header
# @see https://cmake.org/cmake/help/latest/prop_tgt/WINDOWS_EXPORT_ALL_SYMBOLS.html#prop_tgt:WINDOWS_EXPORT_ALL_SYMBOLS
# if(MSVC)
    # @see https://blog.kitware.com/create-dlls-on-windows-without-declspec-using-new-cmake-export-all-feature/
    # @see https://cmake.org/cmake/help/v3.3/module/GenerateExportHeader.html
    #
    # set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
    # set(BUILD_SHARED_LIBS TRUE)
# endif()

# Generate export macros for libraries
# @see https://gernotklingler.com/blog/creating-using-shared-libraries-different-compilers-different-operating-systems/
generate_export_header(${LIB_NAME}
                        BASE_NAME XEVE
                        EXPORT_FILE_NAME ${CMAKE_BINARY_DIR}/xeve_exports.h)

# This will cause the export macros to expand to nothing when building the static library.
set_target_properties(${LIB_NAME} PROPERTIES  COMPILE_FLAGS -DLIBSHARED_AND_STATIC_STATIC_DEFINE)

source_group("base\\header" FILES ${LIB_BASE_INC} ${ETM_INC})
source_group("base\\source" FILES ${LIB_BASE_SRC} ${LIB_API_SRC})
source_group("base\\sse\\header" FILES ${LIB_SSE_INC})
source_group("base\\sse\\source" FILES ${LIB_SSE_SRC})
source_group("base\\avx\\header" FILES ${LIB_AVX_INC})
source_group("base\\avx\\source" FILES ${LIB_AVX_SRC})
source_group("main\\header" FILES ${LIB_MAIN_INC})
source_group("main\\source" FILES ${LIB_MAIN_SRC} ${LIB_API_MAIN_SRC})
source_group("main\\sse\\header" FILES ${LIB_MAIN_SSE_INC})
source_group("main\\sse\\source" FILES ${LIB_MAIN_SSE_SRC})
source_group("main\\avx\\header" FILES ${LIB_MAIN_AVX_INC})
source_group("main\\avx\\source" FILES ${LIB_MAIN_AVX_SRC})
source_group("base\\neon\\header" FILES ${LIB_NEON_INC})
source_group("base\\neon\\source" FILES ${LIB_NEON_SRC})

if("${ARM}" STREQUAL "TRUE")
  include_directories( ${LIB_NAME} PUBLIC . .. ../inc ../src_base ../src_base/neon)
else()
  include_directories( ${LIB_NAME} PUBLIC . .. ../inc ./sse ./avx ../src_base ../src_base/sse ../src_base/avx)
endif()

set( SSE ${BASE_INC_FILES} ${LIB_SSE_SRC} ${LIB_MAIN_SSE_SRC})
set( AVX ${LIB_AVX_SRC} ${LIB_MAIN_AVX_SRC})

set_target_properties(${LIB_NAME}_dynamic PROPERTIES OUTPUT_NAME ${LIB_NAME})

if( MSVC )
    target_compile_definitions( ${LIB_NAME} PUBLIC ANY _CRT_SECURE_NO_WARNINGS )
    target_compile_definitions( ${LIB_NAME}_dynamic PUBLIC ANY _CRT_SECURE_NO_WARNINGS )

    # Since both the import library associated with DLL and the static library have the same names
    # they must be build in different locations to avoid overwriting.
    #
    set_target_properties(${LIB_NAME} PROPERTIES FOLDER lib
                            ARCHIVE_OUTPUT_DIRECTORY  ${CMAKE_CURRENT_BINARY_DIR}/static/lib)

    set_target_properties(${LIB_NAME}_dynamic PROPERTIES FOLDER lib
                            ARCHIVE_OUTPUT_DIRECTORY  ${CMAKE_CURRENT_BINARY_DIR}/import/lib)

elseif( UNIX OR MINGW )
    if("${ARM}" STREQUAL "FALSE")
        set_property( SOURCE ${SSE} APPEND PROPERTY COMPILE_FLAGS "-msse4.1" )
        set_property( SOURCE ${AVX} APPEND PROPERTY COMPILE_FLAGS " -mavx2" )
    endif()

    set_target_properties(${LIB_NAME}_dynamic PROPERTIES FOLDER lib LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

    target_compile_definitions( ${LIB_NAME} PUBLIC ANY LINUX )
    target_link_libraries(${LIB_NAME} m)
endif()

# Install rules
#
# Set DCMAKE_INSTALL_PREFIX to change default install prefix
# e.g cmake .. -DCMAKE_INSTALL_PREFIX='C:\Users\Samsung\git\xeve_internal\build-windows\install'

# List the headers we want to declare as public for installation.
set(XEVE_PUBLIC_HEADERS "${ETM_INC}")
set_target_properties(${LIB_NAME} PROPERTIES PUBLIC_HEADER "${XEVE_PUBLIC_HEADERS}")
set_target_properties(${LIB_NAME}_dynamic PROPERTIES PUBLIC_HEADER "${XEVE_PUBLIC_HEADERS}")

set(XEVE_PRIVATE_HEADERS "${LIB_BASE_INC}" "${LIB_SSE_INC}" "${LIB_AVX_INC}")

include(GNUInstallDirs)

# Install static library and public headers
#
# Static library (libxeveb.a or xeveb.lib) will be installed to <prefix>/lib/xeveb
# Public headers will be installed to <prefix>/include/xeveb
#
install(TARGETS ${LIB_NAME}
        ARCHIVE COMPONENT Development DESTINATION ${CMAKE_INSTALL_LIBDIR}/${LIB_NAME}
        PUBLIC_HEADER COMPONENT Development DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${LIB_NAME}
)

# Install shared library
#
# On non-DLL platforms shared library (libxeveb.so) will be installed to <prefix>/lib/xeveb.
# On DLL platforms the shared DLL (xeveb.dll) will be installed to <prefix>/bin and its import library will be installed to <prefix>/lib/xeveb/import
#
install(TARGETS ${LIB_NAME}_dynamic
        RUNTIME COMPONENT Libraries DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY
             COMPONENT Libraries DESTINATION ${CMAKE_INSTALL_LIBDIR}
             NAMELINK_COMPONENT Development DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE COMPONENT Development DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER COMPONENT Development DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${LIB_NAME}
)

install( FILES
    ${PROJECT_BINARY_DIR}/xeve_exports.h COMPONENT Development DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${LIB_NAME}
)

configure_file(
    "${CMAKE_SOURCE_DIR}/pkgconfig/${LIB_NAME}.pc.in"
    "${CMAKE_BINARY_DIR}/${LIB_NAME}.pc"
    IMMEDIATE @ONLY)

install( FILES
    "${CMAKE_BINARY_DIR}/${LIB_NAME}.pc" COMPONENT Development DESTINATION  ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
