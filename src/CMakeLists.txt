cmake_minimum_required(VERSION 3.8)

option(USE_GTK "Use GTK over Curses" ON)

find_package(Threads)

set(SOURCES
	${PROJECT_SOURCE_DIR}/src/data/Session.cpp
	${PROJECT_SOURCE_DIR}/src/data/CurrentState.cpp
	${PROJECT_SOURCE_DIR}/src/connection/Connection.cpp
	${PROJECT_SOURCE_DIR}/src/config/config.cpp
	${PROJECT_SOURCE_DIR}/src/math/Vector3D.cpp
	${PROJECT_SOURCE_DIR}/src/main.cpp
)

set(HEADERS
	${PROJECT_SOURCE_DIR}/src/data/Session.h
	${PROJECT_SOURCE_DIR}/src/data/CurrentState.h
	${PROJECT_SOURCE_DIR}/src/connection/Connection.h
	${PROJECT_SOURCE_DIR}/src/config/config.h
	${PROJECT_SOURCE_DIR}/src/math/Vector3D.h
	${PROJECT_SOURCE_DIR}/src/ui/ui.h
)

if (USE_GTK)
	find_package(PkgConfig REQUIRED)
	PKG_CHECK_MODULES(GTK3 REQUIRED gtk+-3.0)

	include_directories(${GTK3_INCLUDE_DIRS})
	link_directories(${GTK3_LIBRARY_DIRS})
	add_definitions(${GTK3_CFLAGS_OTHER})

	list(APPEND SOURCES
		${PROJECT_SOURCE_DIR}/src/ui/gtk.cpp
	)
else()
	if(WIN32)
		set(PDCURSES_SOURCES
			${PROJECT_SOURCE_DIR}/PDCurses/
		)
		# Use PDCurses on Windows, as NCurses doesn't work in MinGW
		include_directories(${PROJECT_SOURCE_DIR}/PDCurses)
	else()
		# Use NCurses on Linux
		find_package(Curses REQUIRED)
		include_directories(${CURSES_INCLUDE_DIR})
	endif()

	list(APPEND SOURCES
		${PROJECT_SOURCE_DIR}/src/ui/curses.cpp
		${PROJECT_SOURCE_DIR}/src/ui/commands/arm.cpp
		${PROJECT_SOURCE_DIR}/src/ui/commands/close.cpp
		${PROJECT_SOURCE_DIR}/src/ui/commands/continue.cpp
		${PROJECT_SOURCE_DIR}/src/ui/commands/disarm.cpp
		${PROJECT_SOURCE_DIR}/src/ui/commands/open.cpp
		${PROJECT_SOURCE_DIR}/src/ui/commands/set.cpp
	)

	list(APPEND HEADERS
		${PROJECT_SOURCE_DIR}/src/ui/commands/commands.h
	)
endif()

add_executable(uas4stem-logger ${SOURCES} ${HEADERS})

target_link_libraries(uas4stem-logger ${CMAKE_THREAD_LIBS_INIT})

if (USE_GTK)
	target_link_libraries(uas4stem-logger ${GTK3_LIBRARIES})
elseif(WIN32)
else()
	target_link_libraries(uas4stem-logger ${CURSES_LIBRARIES})
endif()
