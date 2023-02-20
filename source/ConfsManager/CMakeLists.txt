cmake_minimum_required(VERSION 3.10)
project(ConfsManager)

# Get the name of the library
set(LIBRARY_NAME ${PROJECT_NAME})

# Concatenate the path and name to get the full path of the library binary
set(LIBRARY_BINARY ${CMAKE_BINARY_DIR}/${PROJECT_NAME}/${CMAKE_SHARED_LIBRARY_PREFIX}${LIBRARY_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX})

# Add ConfsManager as first Lib
add_library(${PROJECT_NAME} SHARED ${PROJECT_SOURCE_DIR}/${PROJECT_NAME}.cpp)

# Include Json folder to ConfsManager lib
target_include_directories(ConfsManager PRIVATE ../Json/)

# Add the exported symbols
if(WIN32)
  set_target_properties(${PROJECT_NAME} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
  add_definitions(-D_EXP_CONFS)
  target_link_libraries(${PROJECT_NAME} Json)
else()
  set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath,$ORIGIN")
  target_link_libraries(${PROJECT_NAME} PRIVATE pthread Json)
endif()

if(WIN32)  
  install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION "${CMAKE_SOURCE_DIR}/_install/windows/Configs")  
else()
  install(FILES ${LIBRARY_BINARY} DESTINATION "${CMAKE_SOURCE_DIR}/_install/linux/Configs")  
endif()