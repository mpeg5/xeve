include_directories (${PROJECT_SOURCE_DIR}/app)
include_directories (${CMAKE_BINARY_DIR})
set( EXE_NAME xeve_app )
set( EXE_NAME_BASE xeveb_app)

file (GLOB ENC_SRC "*.c" )
file (GLOB ENC_INC "*.h" )

if(("${SET_PROF}" STREQUAL "MAIN"))
  add_executable (${EXE_NAME} ${ENC_SRC} ${ENC_INC} )
  include_directories( ${EXE_NAME} PUBLIC . .. ../inc ../src_base ../src_main ${BASE_SRC_PATH})
  
  if(XEVE_APP_STATIC_BUILD)
    target_link_libraries (${EXE_NAME} xeve)
  else()
    target_link_libraries (${EXE_NAME} xeve_dynamic)
  endif()
  
  set_property(TARGET ${EXE_NAME} PROPERTY FOLDER "app")
  set_target_properties(${EXE_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
  
  if( MSVC )
    target_compile_definitions( ${EXE_NAME} PUBLIC _CRT_SECURE_NO_WARNINGS ANY )
  elseif( UNIX OR MINGW )
    target_compile_definitions( ${EXE_NAME} PUBLIC LINUX ANY )
    target_link_libraries (${EXE_NAME} m)
  endif()

  # Install rules
  #
  # Install executable to <prefix>/bin
  include( GNUInstallDirs)
  install( TARGETS ${EXE_NAME} RUNTIME COMPONENT Runtime DESTINATION ${CMAKE_INSTALL_BINDIR})

endif()

if(("${SET_PROF}" STREQUAL "BASE"))
  add_executable (${EXE_NAME_BASE} ${ENC_SRC} ${ENC_INC} )
  include_directories( ${EXE_NAME_BASE} PUBLIC . .. ../inc ../src_base ${BASE_SRC_PATH})
  
    if(XEVE_APP_STATIC_BUILD)
    target_link_libraries (${EXE_NAME_BASE} xeveb)
  else()
    target_link_libraries (${EXE_NAME_BASE} xeveb_dynamic)
  endif()

  set_property(TARGET ${EXE_NAME_BASE} PROPERTY FOLDER "app")
  set_target_properties(${EXE_NAME_BASE} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
  if( MSVC )
    target_compile_definitions( ${EXE_NAME_BASE} PUBLIC _CRT_SECURE_NO_WARNINGS ANY )
  elseif( UNIX OR MINGW )
    target_compile_definitions( ${EXE_NAME_BASE} PUBLIC LINUX ANY )
    target_link_libraries (${EXE_NAME_BASE} m)
  endif()
  
  # Install rules
  #
  # Install executable to <prefix>/bin
  include( GNUInstallDirs)
  install( TARGETS ${EXE_NAME_BASE} RUNTIME COMPONENT Runtime DESTINATION ${CMAKE_INSTALL_BINDIR}) 

endif()

