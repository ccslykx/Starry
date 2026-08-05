cmake_minimum_required(VERSION 3.18)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)

project(Starry VERSION 0.3.0)

include(CTest)

set(STARRY_GENERATED_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${STARRY_GENERATED_INCLUDE_DIR}")
configure_file(
	include/Core/StarryVersion.h.in
	"${STARRY_GENERATED_INCLUDE_DIR}/StarryVersion.h"
	@ONLY
)

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
	set(WINDOWS_SELECTION_FILE
		include/Core/Platform/WinSelectionProvider.h
		src/core/platform/WinSelectionProvider.cpp
	)
	set(PLATFORM_FILE
		include/Core/Platform/WinMouseListener.h
		src/core/platform/WinMouseListener.cpp
		${WINDOWS_SELECTION_FILE}
	)
elseif (CMAKE_HOST_APPLE)
	set(MAC_PERMISSION_FILE
		include/Core/Platform/MacAccessibilityPermission.h
		src/core/platform/MacAccessibilityPermission.cpp
	)
	set(PLATFORM_FILE
		${MAC_PERMISSION_FILE}
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

if (APPLE)
	set_target_properties(${PROJECT_NAME} PROPERTIES
		MACOSX_BUNDLE TRUE
		MACOSX_BUNDLE_BUNDLE_NAME "Starry"
		MACOSX_BUNDLE_GUI_IDENTIFIER "com.ccslykx.starry"
		MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
		MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
		MACOSX_BUNDLE_INFO_STRING "Starry ${PROJECT_VERSION}"
		MACOSX_BUNDLE_COPYRIGHT "Copyright © Ccslykx"
		MACOSX_BUNDLE_INFO_PLIST
			"${CMAKE_CURRENT_SOURCE_DIR}/cmake/Info.plist.in"
	)
endif()

target_include_directories(${PROJECT_NAME} PUBLIC
	${STARRY_GENERATED_INCLUDE_DIR}
	include
	include/Core
	include/SGui
	include/Core/Platform
)

find_package(Qt6 COMPONENTS Core Widgets Concurrent Gui LinguistTools REQUIRED)

set(LIBRARYS Qt6::Core Qt6::Widgets	Qt6::Concurrent	Qt6::Gui)

if (CMAKE_HOST_WIN32)
	set(WINDOWS_SELECTION_LIBRARIES ole32 oleaut32 uiautomationcore)
	list(APPEND LIBRARYS ${WINDOWS_SELECTION_LIBRARIES})
endif()

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

if (CMAKE_HOST_APPLE)
	find_library(APPLICATION_SERVICES_FRAMEWORK ApplicationServices REQUIRED)
	target_link_libraries(${PROJECT_NAME} PUBLIC
		${APPLICATION_SERVICES_FRAMEWORK}
	)
endif()

if (BUILD_TESTING)
	find_package(Qt6 COMPONENTS Test REQUIRED)

	add_executable(StarryPluginCommandTests
		tests/SPluginCommandTests.cpp
		include/Core/SPluginCommand.h
		src/core/SPluginCommand.cpp
	)
	target_include_directories(StarryPluginCommandTests PRIVATE
		include
		include/Core
	)
	target_link_libraries(StarryPluginCommandTests PRIVATE
		Qt6::Core
		Qt6::Test
	)
	add_test(NAME StarryPluginCommandTests COMMAND StarryPluginCommandTests)

	add_executable(StarryConfigTests
		tests/SConfigTests.cpp
		include/Core/SConfig.h
		include/Core/SPluginInfo.h
		src/core/SConfig.cpp
		src/core/SPluginInfo.cpp
		${RESOURCES}
	)
	target_include_directories(StarryConfigTests PRIVATE
		${STARRY_GENERATED_INCLUDE_DIR}
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
		include/Core/SLanguageManager.h
		include/Core/SPluginInfo.h
		include/Core/SPluginCommand.h
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
		src/core/SLanguageManager.cpp
		src/core/SPluginInfo.cpp
		src/core/SPluginCommand.cpp
		src/core/SPluginTaskManager.cpp
		src/core/SSelection.cpp
		src/gui/SButton.cpp
		src/gui/SPluginEditor.cpp
		src/gui/SPluginItem.cpp
		src/gui/SPopup.cpp
		src/gui/SPopupItem.cpp
		src/gui/SSettings.cpp
		src/gui/SSwitcher.cpp
		${MAC_PERMISSION_FILE}
		${WINDOWS_SELECTION_FILE}
		${RESOURCES}
	)
	target_include_directories(StarrySettingsTests PRIVATE
		${STARRY_GENERATED_INCLUDE_DIR}
		include
		include/Core
		include/SGui
	)
	target_link_libraries(StarrySettingsTests PRIVATE
		Qt6::Core
		Qt6::Concurrent
		Qt6::Gui
		Qt6::Widgets
		Qt6::Test
		${APPLICATION_SERVICES_FRAMEWORK}
	)
	if (CMAKE_HOST_WIN32)
		target_link_libraries(StarrySettingsTests PRIVATE
			${WINDOWS_SELECTION_LIBRARIES}
		)
	endif()
	add_test(NAME StarrySettingsTests COMMAND StarrySettingsTests)
	set_tests_properties(StarrySettingsTests PROPERTIES
		ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
	)
endif()

set(STARRY_TRANSLATION_FILES
	translations/starry_zh_CN.ts
	translations/starry_zh_TW.ts
	translations/starry_de.ts
	translations/starry_fr.ts
	translations/starry_ja.ts
)

set(STARRY_TRANSLATION_TARGETS Starry)
if(BUILD_TESTING)
	list(APPEND STARRY_TRANSLATION_TARGETS StarrySettingsTests)
endif()

qt_add_translations(
	TARGETS ${STARRY_TRANSLATION_TARGETS}
	SOURCE_TARGETS Starry
	TS_FILES ${STARRY_TRANSLATION_FILES}
	MERGE_QT_TRANSLATIONS
	QT_TRANSLATION_CATALOGS qtbase
	RESOURCE_PREFIX "/i18n"
)
