cmake_minimum_required(VERSION 3.10)
project(Core)

# Get the name of the library
set(LIBRARY_NAME ${PROJECT_NAME})

# Concatenate the path and name to get the full path of the library binary
set(LIBRARY_BINARY ${CMAKE_BINARY_DIR}/${PROJECT_NAME}/${CMAKE_SHARED_LIBRARY_PREFIX}${LIBRARY_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX})

# Add the Core library
if(WIN32)
  add_library(Core SHARED ${PROJECT_SOURCE_DIR}/Http.cpp ${PROJECT_SOURCE_DIR}/Http.def)  
elseif(UNIX AND NOT APPLE)
  add_library(Core SHARED ${PROJECT_SOURCE_DIR}/Http.cpp)  
  set_target_properties(Core PROPERTIES LINK_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--version-script=${PROJECT_SOURCE_DIR}/Http.map,-rpath,$ORIGIN")
endif()

# Include Parser folder to Creator lib
target_include_directories(${PROJECT_NAME} PRIVATE ../Parser/)

# Include Router
target_include_directories(${PROJECT_NAME} PRIVATE ../Router/)

# Link ConfsManager to Core 
target_link_libraries(Core PRIVATE ConfsManager Parser Router)

if(WIN32)  
  install(TARGETS Core RUNTIME DESTINATION "${CMAKE_SOURCE_DIR}/_install/windows/Core")  
else()
  install(FILES ${LIBRARY_BINARY} DESTINATION "${CMAKE_SOURCE_DIR}/_install/linux/Core")  
endif()