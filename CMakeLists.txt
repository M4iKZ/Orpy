cmake_minimum_required(VERSION 3.10)
project(Orpy)

# set c++ standard to c++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set the build type to release
set(CMAKE_BUILD_TYPE Release)
# set(CMAKE_BUILD_TYPE Debug)
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")

# Add the common include directory to the include search path
include_directories(${CMAKE_SOURCE_DIR}/source)

# Add the core executable
add_executable(Orpy ${PROJECT_SOURCE_DIR}/source/Orpy.cpp
			        ${PROJECT_SOURCE_DIR}/source/Core/core.cpp
			        ${PROJECT_SOURCE_DIR}/source/Sockets/sockets.cpp
				    ${PROJECT_SOURCE_DIR}/source/Conf/conf.cpp
			        ${PROJECT_SOURCE_DIR}/source/Parser/parser.cpp
			        ${PROJECT_SOURCE_DIR}/source/Router/router.cpp
			        ${PROJECT_SOURCE_DIR}/source/Json/json.cpp
			        ${PROJECT_SOURCE_DIR}/source/Tools/tools.cpp)

if(UNIX AND NOT APPLE)
  target_link_libraries(Orpy -ldl)  
  set_target_properties(Orpy PROPERTIES LINK_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath,$ORIGIN/Core")
endif()

if(WIN32)  
  install(TARGETS Orpy RUNTIME DESTINATION "${CMAKE_SOURCE_DIR}/_install/windows")    
else()
  install(TARGETS Orpy RUNTIME DESTINATION "${CMAKE_SOURCE_DIR}/_install/linux")
endif()