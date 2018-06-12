cmake_minimum_required(VERSION 2.8)

project(SauerProxy)

file(GLOB_RECURSE sources ${PROJECT_SOURCE_DIR}/src/**.cpp ${PROJECT_SOURCE_DIR}/src/**.h)
add_executable(${PROJECT_NAME} ${sources})
add_subdirectory(${CMAKE_SOURCE_DIR}/enet/)

INCLUDE_DIRECTORIES(src ${PROJECT_SOURCE_DIR}/enet/include)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} enet)

