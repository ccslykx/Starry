cmake_minimum_required(VERSION 3.18)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)

project(Starry VERSION 0.1.0)

file(GLOB HEADERS CMAKE_CONFIGURE_DEPENDS
    include/*.h
    include/Core/*.h
    include/SGui/*.h
)
file(GLOB SOURCES CMAKE_CONFIGURE_DEPENDS
    src/*.cpp
    src/core/*.cpp
    src/gui/*.cpp
)
file(GLOB RESOURCES CMAKE_CONFIGURE_DEPENDS
    src/resources/*.qrc
)

find_package(Qt6 COMPONENTS Core Widgets Concurrent Gui REQUIRED)

add_executable(${PROJECT_NAME}
    ${HEADERS}
    ${SOURCES}
    ${RESOURCES}
)

target_include_directories(${PROJECT_NAME} PUBLIC
    include
    include/Core
    include/SGui
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    Qt6::Core
    Qt6::Widgets
    Qt6::Concurrent
    Qt6::Gui
    -lX11 
    -lXtst # https://stackoverflow.com/questions/66906557/linking-target-failed-for-lib-with-x11-c
)
