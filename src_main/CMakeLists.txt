set( LIB_NAME xeve )

file (GLOB ETM_INC "../inc/*.h")
file (GLOB LIB_BASE_SRC "../src_base/xeve_*.c")
file (GLOB LIB_BASE_INC "../src_base/xeve_*.h" )
file (GLOB LIB_SSE_SRC "../src_base/sse/xeve_*.c")
file (GLOB LIB_SSE_INC "../src_base/sse/xeve_*.h" )
file (GLOB LIB_AVX_SRC "../src_base/avx/xeve_*.c")
file (GLOB LIB_AVX_INC "../src_base/avx/xeve_*.h" )
file (GLOB LIB_API_MAIN_SRC "./xevem.c")
file (GLOB LIB_MAIN_SRC "./xevem_*.c")
file (GLOB LIB_MAIN_INC "./xevem_*.h" )
file (GLOB LIB_MAIN_SSE_SRC "./sse/xevem_*.c")
file (GLOB LIB_MAIN_SSE_INC "./sse/xevem_*.h" )


add_library( ${LIB_NAME} STATIC ${LIB_API_MAIN_SRC} ${ETM_INC} ${LIB_BASE_SRC} ${LIB_BASE_INC} ${LIB_MAIN_SRC} ${LIB_MAIN_INC} 
                                ${LIB_SSE_SRC} ${LIB_SSE_INC} ${LIB_MAIN_SSE_SRC} ${LIB_MAIN_SSE_INC} ${LIB_AVX_SRC} ${LIB_AVX_INC} )
add_library( ${LIB_NAME}_dynamic SHARED ${LIB_API_MAIN_SRC} ${ETM_INC} ${LIB_BASE_SRC} ${LIB_BASE_INC} ${LIB_MAIN_SRC} ${LIB_MAIN_INC} 
                                        ${LIB_SSE_SRC} ${LIB_SSE_INC} ${LIB_MAIN_SSE_SRC} ${LIB_MAIN_SSE_INC} ${LIB_AVX_SRC} ${LIB_AVX_INC} )

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

include_directories( ${LIB_NAME} PUBLIC . .. ../inc ./sse ../src_base ../src_base/sse ../src_base/avx)

set_target_properties(${LIB_NAME} PROPERTIES FOLDER lib
                                               ARCHIVE_OUTPUT_DIRECTORY  ${CMAKE_BINARY_DIR}/lib)
                                               
set_target_properties(${LIB_NAME_BASE} PROPERTIES FOLDER lib
                                               ARCHIVE_OUTPUT_DIRECTORY  ${CMAKE_BINARY_DIR}/lib)

set( SSE ${BASE_INC_FILES} ${LIB_SSE_SRC} ${LIB_MAIN_SSE_SRC})
set( AVX ${LIB_AVX_SRC} )
set_target_properties(${LIB_NAME}_dynamic PROPERTIES OUTPUT_NAME ${LIB_NAME})

if( MSVC )                      
  target_compile_definitions( ${LIB_NAME} PUBLIC ANY _CRT_SECURE_NO_WARNINGS )
  set_target_properties(${LIB_NAME}_dynamic PROPERTIES FOLDER lib
                                               RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
elseif( UNIX OR MINGW )
  set_target_properties(${LIB_NAME}_dynamic PROPERTIES FOLDER lib
                                               LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
  target_compile_definitions( ${LIB_NAME} PUBLIC ANY LINUX )  
  target_link_libraries(${LIB_NAME} m)
  set_property( SOURCE ${SSE} APPEND PROPERTY COMPILE_FLAGS "-msse4.1" )
  set_property( SOURCE ${AVX} APPEND PROPERTY COMPILE_FLAGS " -mavx2" )
endif()
