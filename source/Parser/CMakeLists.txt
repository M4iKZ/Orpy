cmake_minimum_required(VERSION 3.10)
project(Parser)

# Get the name of the library
set(LIBRARY_NAME ${PROJECT_NAME})

# Concatenate the path and name to get the full path of the library binary
set(LIBRARY_BINARY ${CMAKE_BINARY_DIR}/${PROJECT_NAME}/${CMAKE_SHARED_LIBRARY_PREFIX}${LIBRARY_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX})

add_library(Parser SHARED ${PROJECT_SOURCE_DIR}/Parser.cpp)

# Add the exported symbols
if(WIN32)
  set_target_properties(Parser PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
elseif(UNIX AND NOT APPLE)
  set_target_properties(Parser PROPERTIES LINK_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath,$ORIGIN")  
endif()

# Include ConfsManager folder to Parser lib
target_include_directories(Parser PRIVATE ../ConfsManager/)

# Link ConfsManager to Parser 
target_link_libraries(Parser PRIVATE ConfsManager)

if(WIN32)  
  install(TARGETS Parser RUNTIME DESTINATION "${CMAKE_SOURCE_DIR}/_install/windows/Core")  
else()
  install(FILES ${LIBRARY_BINARY} DESTINATION "${CMAKE_SOURCE_DIR}/_install/linux/Core")  
endif()