cmake_minimum_required(VERSION 3.18)

set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 17)

project(starry VERSION 0.1.0)

file(GLOB HEADERS src/*.h)
file(GLOB SOURCES src/*.cpp)

find_package(Qt6 COMPONENTS Core Widgets Concurrent Gui REQUIRED)

add_executable(${PROJECT_NAME}
    ${HEADERS}
    ${SOURCES}
    src/resources/resources.qrc
)

target_link_libraries(${PROJECT_NAME}
    Qt6::Core
    Qt6::Widgets
    Qt6::Concurrent
    Qt6::Gui
    -lX11 
    -lXtst # https://stackoverflow.com/questions/66906557/linking-target-failed-for-lib-with-x11-c
)