cmake_minimum_required(VERSION 3.10)
project(fileGuard)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Get the name of the library
set(LIBRARY_NAME ${PROJECT_NAME})

# Concatenate the path and name to get the full path of the library binary
set(LIBRARY_BINARY ${CMAKE_BINARY_DIR}/${PROJECT_NAME}/${CMAKE_SHARED_LIBRARY_PREFIX}${LIBRARY_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX})

add_library(${PROJECT_NAME} SHARED ${PROJECT_SOURCE_DIR}/fileGuard.cpp)

# Add the exported symbols
if(WIN32)
  add_definitions(-D_EXP_FGUARD)
  set_target_properties(${PROJECT_NAME} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)  
endif()
  
if(WIN32)
  install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION "${CMAKE_SOURCE_DIR}/_install/windows/Tools") 
else()
  install(FILES ${LIBRARY_BINARY} DESTINATION "${CMAKE_SOURCE_DIR}/_install/linux/Tools")  
endif()