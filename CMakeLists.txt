cmake_minimum_required(VERSION 3.18)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)

project(Starry VERSION 0.2.0)

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

if (CMAKE_HOST_LINUX)
	set(PLATFORM_FILE
		include/Core/Platform/X11MouseListener.h
		src/core/platform/X11MouseListener.cpp
	)
    target_link_libraries(${PROJECT_NAME} PUBLIC -lX11 -lXtst) # https://stackoverflow.com/questions/66906557/linking-target-failed-for-lib-with-x11-c
elseif (CMAKE_HOST_WIN32)
	set(PLATFORM_FILE
		include/Core/Platform/WinMouseListener.h
		src/core/platform/WinMouseListener.cpp
	)
elseif (CMAKE_HOST_APPLE)
	set(PLATFORM_FILE
		include/Core/Platform/MacMouseListener.h
		src/core/platform/MacMouseListener.cpp
	)
endif()

add_executable(${PROJECT_NAME}
    ${HEADERS}
    ${SOURCES}
    ${RESOURCES}
	${PLATFORM_FILE}
)

find_package(Qt6 COMPONENTS Core Widgets Concurrent Gui REQUIRED)

target_include_directories(${PROJECT_NAME} PUBLIC
    include
    include/Core
    include/SGui
	include/Core/Platform
)

target_link_libraries(${PROJECT_NAME} PUBLIC
	Qt6::Core
	Qt6::Widgets
	Qt6::Concurrent
	Qt6::Gui
)
