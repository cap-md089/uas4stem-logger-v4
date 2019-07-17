COMPILE := g++ -g -c -Wall
LINK := g++ -Wall

GTK_CFLAGS := $(shell pkg-config --cflags gtk+-3.0)
GTK_LIBS := $(shell pkg-config --libs gtk+-3.0)

NCURSES_CFLAGS := $(shell pkg-config --cflags ncurses)
NCURSES_LIBS := $(shell pkg-config --libs ncurses)

OS_LIBS :=
ifdef OS
	OS_LIBS := -lwinmm -lws2_32 -lshlwapi
else
	OS_LIBS := -pthread
endif

vim: out/main-vim
gui: out/main
all: vim gui

out/main-vim: out/cursesui.o out/math.o out/main.o out/currentstate.o out/connection.o out/config.o out/session.o out/command_arm.o
	$(LINK) $(NCURSES_CFLAGS) -o out/main-vim out/math.o out/config.o out/currentstate.o out/connection.o out/session.o out/command_arm.o out/cursesui.o out/main.o $(NCURSES_LIBS) $(OS_LIBS)

out/main: out/gtkui.o out/math.o out/main.o out/currentstate.o out/connection.o out/config.o out/session.o
	$(LINK) $(GTK_CFLAGS) -o out/main out/math.o out/config.o out/currentstate.o out/connection.o out/session.o out/gtkui.o out/main.o $(GTK_LIBS) $(OS_LIBS)

out/math.o: src/math/Vector3D.cpp
	$(COMPILE) -o out/math.o src/math/Vector3D.cpp

out/main.o: src/main.cpp
	$(COMPILE) -o out/main.o src/main.cpp

out/currentstate.o: src/data/CurrentState.cpp
	$(COMPILE) -o out/currentstate.o src/data/CurrentState.cpp -lwinmm

out/gtkui.o: src/ui/gtk.cpp
	$(COMPILE) $(GTK_CFLAGS) -o out/gtkui.o src/ui/gtk.cpp $(GTK_LIBS)

out/cursesui.o: src/ui/curses.cpp
	$(COMPILE) $(NCURSES_CFLAGS) -o out/cursesui.o src/ui/curses.cpp $(NCURSES_LIBS)

out/connection.o: src/connection/Connection.cpp
	$(COMPILE) -o out/connection.o -pthread src/connection/Connection.cpp -lws2_32

out/config.o: src/config/config.cpp
	$(COMPILE) -o out/config.o src/config/config.cpp

out/session.o: src/data/Session.cpp
	$(COMPILE) -o out/session.o src/data/Session.cpp

out/command_arm.o: src/ui/commands/arm.cpp
	$(COMPILE) -o out/command_arm.o src/ui/commands/arm.cpp

clean:
	rm out/*
