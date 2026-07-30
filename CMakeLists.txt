cmake_minimum_required(VERSION 3.18)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)

project(Starry VERSION 0.2.0)

include(CTest)

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
		include/Core/Platform/WaylandMouseListener.h
		src/core/platform/WaylandMouseListener.cpp
	)
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

target_include_directories(${PROJECT_NAME} PUBLIC
	include
	include/Core
	include/SGui
	include/Core/Platform
)

find_package(Qt6 COMPONENTS Core Widgets Concurrent Gui REQUIRED)

set(LIBRARYS Qt6::Core Qt6::Widgets	Qt6::Concurrent	Qt6::Gui)

find_package(Libinput)

if (CMAKE_HOST_LINUX)
# https://stackoverflow.com/questions/66906557/linking-target-failed-for-lib-with-x11-c
set(LIBRARYS
	${LIBRARYS}
	Libinput::Libinput # or -linput
	-ludev
	-lX11 
	-lXtst
)
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC ${LIBRARYS})

if (BUILD_TESTING)
	find_package(Qt6 COMPONENTS Test REQUIRED)

	add_executable(StarryConfigTests
		tests/SConfigTests.cpp
		include/Core/SConfig.h
		include/Core/SPluginInfo.h
		src/core/SConfig.cpp
		src/core/SPluginInfo.cpp
		${RESOURCES}
	)
	target_include_directories(StarryConfigTests PRIVATE
		include
		include/Core
		include/SGui
	)
	target_link_libraries(StarryConfigTests PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets
		Qt6::Test
	)
	add_test(NAME StarryConfigTests COMMAND StarryConfigTests)
	set_tests_properties(StarryConfigTests PROPERTIES
		ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
	)

	add_executable(StarrySettingsTests
		tests/SSettingsTests.cpp
		include/Core/SConfig.h
		include/Core/SPluginInfo.h
		include/Core/SPluginTaskManager.h
		include/Core/SSelection.h
		include/SGui/SButton.h
		include/SGui/SPluginEditor.h
		include/SGui/SPluginItem.h
		include/SGui/SPopup.h
		include/SGui/SPopupItem.h
		include/SGui/SSettings.h
		include/SGui/SSwitcher.h
		src/core/SConfig.cpp
		src/core/SPluginInfo.cpp
		src/core/SPluginTaskManager.cpp
		src/core/SSelection.cpp
		src/gui/SButton.cpp
		src/gui/SPluginEditor.cpp
		src/gui/SPluginItem.cpp
		src/gui/SPopup.cpp
		src/gui/SPopupItem.cpp
		src/gui/SSettings.cpp
		src/gui/SSwitcher.cpp
		${RESOURCES}
	)
	target_include_directories(StarrySettingsTests PRIVATE
		include
		include/Core
		include/SGui
	)
	target_link_libraries(StarrySettingsTests PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets
		Qt6::Test
	)
	add_test(NAME StarrySettingsTests COMMAND StarrySettingsTests)
	set_tests_properties(StarrySettingsTests PROPERTIES
		ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
	)
endif()
