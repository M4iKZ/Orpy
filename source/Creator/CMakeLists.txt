cmake_minimum_required(VERSION 3.10)
project(Creator)

# Get the name of the library
set(LIBRARY_NAME ${PROJECT_NAME})

# Concatenate the path and name to get the full path of the library binary
set(LIBRARY_BINARY ${CMAKE_BINARY_DIR}/${PROJECT_NAME}/${CMAKE_SHARED_LIBRARY_PREFIX}${LIBRARY_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX})

# Add the Creator library
if(WIN32)
  add_library(Creator SHARED ${PROJECT_SOURCE_DIR}/Creator.cpp ${PROJECT_SOURCE_DIR}/Creator.def)  
elseif(UNIX AND NOT APPLE)
  add_library(Creator SHARED ${PROJECT_SOURCE_DIR}/Creator.cpp)  
  set_target_properties(Creator PROPERTIES LINK_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--version-script=${PROJECT_SOURCE_DIR}/Creator.map,-rpath,$ORIGIN")  
endif()

# Include Parser folder to Creator lib
target_include_directories(Creator PRIVATE ../Parser/)

# Link Parser to Creator 
target_link_libraries(Creator PRIVATE Parser)

# Link Router to Creator
target_link_libraries(Creator PRIVATE Router)

# Link Router to Creator
target_link_libraries(Creator PRIVATE TemplateManager)

# Link Parser to Creator 
#target_link_libraries(Creator PRIVATE Compressor)

# Include Parser Compressor to Creator lib
#target_include_directories(Creator PRIVATE ../Compressor/)

# Include Router folder to Creator lib
target_include_directories(Creator PRIVATE ../Router/)

# Include Template folder to Creator lib
target_include_directories(Creator PRIVATE ../Template/)

if(WIN32)  
  install(TARGETS Creator RUNTIME DESTINATION "${CMAKE_SOURCE_DIR}/_install/windows/Core")  
else()
  install(FILES ${LIBRARY_BINARY} DESTINATION "${CMAKE_SOURCE_DIR}/_install/linux/Core")  
endif()