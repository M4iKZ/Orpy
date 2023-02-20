cmake_minimum_required(VERSION 3.10)
project(TemplateManager)

# Get the name of the library
set(LIBRARY_NAME ${PROJECT_NAME})

# Concatenate the path and name to get the full path of the library binary
set(LIBRARY_BINARY ${CMAKE_BINARY_DIR}/Template/${CMAKE_SHARED_LIBRARY_PREFIX}${LIBRARY_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX})

add_library(TemplateManager SHARED ${PROJECT_SOURCE_DIR}/Template.cpp)

# Add the exported symbols
if(WIN32)
  set_target_properties(TemplateManager PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

if(WIN32)  
  install(TARGETS TemplateManager RUNTIME DESTINATION "${CMAKE_SOURCE_DIR}/_install/windows/Core")  
else()
  install(FILES ${LIBRARY_BINARY} DESTINATION "${CMAKE_SOURCE_DIR}/_install/linux/Core")  
endif()