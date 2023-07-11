cmake_minimum_required(VERSION 3.10)
project(Orpy)

# set c++ standard to c++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set the build type to release
set(CMAKE_BUILD_TYPE Release)
# set(CMAKE_BUILD_TYPE Debug)
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")

# Set the output directory for the build
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/_out)

# Add the common include directory to the include search path
include_directories(${CMAKE_SOURCE_DIR}/Common)

add_subdirectory(ConfsManager)
add_subdirectory(Parser)
add_subdirectory(Router)
add_subdirectory(Sockets)
add_subdirectory(Core)
add_subdirectory(Json)

# Add the core executable
add_executable(Orpy ${PROJECT_SOURCE_DIR}/Orpy/Orpy.cpp)

if(UNIX AND NOT APPLE)
  target_link_libraries(Orpy -ldl)  
  set_target_properties(Orpy PROPERTIES LINK_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath,$ORIGIN/Core")
endif()

if(WIN32)  
  install(TARGETS Orpy RUNTIME DESTINATION "${CMAKE_SOURCE_DIR}/_install/windows")    
else()
  install(TARGETS Orpy RUNTIME DESTINATION "${CMAKE_SOURCE_DIR}/_install/linux")
endif()