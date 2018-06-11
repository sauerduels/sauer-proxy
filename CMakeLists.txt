cmake_minimum_required(VERSION 2.8)

project(SauerProxy)

file(GLOB_RECURSE sources src/**.cpp src/**.h)
add_executable(${PROJECT_NAME} ${sources})

INCLUDE_DIRECTORIES(src ./enet/include)
LINK_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/enet/)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} enet)

