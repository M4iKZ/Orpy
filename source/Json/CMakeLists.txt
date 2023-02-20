cmake_minimum_required(VERSION 3.10)
project(Json)

# Get the name of the library
set(LIBRARY_NAME ${PROJECT_NAME})

# Concatenate the path and name to get the full path of the library binary
set(LIBRARY_BINARY ${CMAKE_BINARY_DIR}/${PROJECT_NAME}/${CMAKE_SHARED_LIBRARY_PREFIX}${LIBRARY_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX})

add_library(Json SHARED ${PROJECT_SOURCE_DIR}/Json.cpp)

# Add the exported symbols
if(WIN32)
  set_target_properties(Json PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

if(WIN32)  
  install(TARGETS Json RUNTIME DESTINATION "${CMAKE_SOURCE_DIR}/_install/windows/Tools")  
else()
  install(FILES ${LIBRARY_BINARY} DESTINATION "${CMAKE_SOURCE_DIR}/_install/linux/Tools")  
endif()