cmake_minimum_required(VERSION 3.10)
project(Sockets)

# Get the name of the library
set(LIBRARY_NAME ${PROJECT_NAME})

# Concatenate the path and name to get the full path of the library binary
set(LIBRARY_BINARY ${CMAKE_BINARY_DIR}/${PROJECT_NAME}/${CMAKE_SHARED_LIBRARY_PREFIX}${LIBRARY_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX})

# Add the Sockets library
if(WIN32)
  add_library(Sockets SHARED ${PROJECT_SOURCE_DIR}/Sockets.cpp ${PROJECT_SOURCE_DIR}/Sockets.def)  
elseif(UNIX AND NOT APPLE)
  add_library(Sockets SHARED ${PROJECT_SOURCE_DIR}/Sockets.cpp)  
  set_target_properties(Sockets PROPERTIES LINK_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--version-script=${PROJECT_SOURCE_DIR}/Sockets.map")  
endif()

# Include Pool folder to Sockets lib
target_include_directories(Sockets PRIVATE ../Pool/)

if(UNIX AND NOT APPLE)
  target_link_libraries(Sockets pthread)
endif()

if(WIN32)  
  install(TARGETS Sockets RUNTIME DESTINATION "${CMAKE_SOURCE_DIR}/_install/windows/Core")  
else()
  install(FILES ${LIBRARY_BINARY} DESTINATION "${CMAKE_SOURCE_DIR}/_install/linux/Core")  
endif()
