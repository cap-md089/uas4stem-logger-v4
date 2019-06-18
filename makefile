COMPILE := g++ -g -c -Wall
LINK := g++ -Wall

GTK_CFLAGS := $(shell pkg-config --cflags gtk+-3.0)
GTK_LIBS := $(shell pkg-config --libs gtk+-3.0)

ifdef OS
	GTK_CFLAGS := $(GTK_CFLAGS) -lwinmm
endif

out/main: out/guimain.o out/math.o out/main.o out/currentstate.o out/connection.o out/config.o out/session.o
	$(LINK) $(GTK_CFLAGS) -o out/main out/math.o out/config.o out/currentstate.o out/connection.o out/session.o out/guimain.o out/main.o $(GTK_LIBS)

out/math.o: src/math/Vector3D.cpp
	$(COMPILE) -o out/math.o src/math/Vector3D.cpp

out/main.o: src/main.cpp
	$(COMPILE) $(GTK_CFLAGS) $(GTK_LIBS) -o out/main.o src/main.cpp

out/currentstate.o: src/data/CurrentState.cpp
	$(COMPILE) -o out/currentstate.o src/data/CurrentState.cpp

out/guimain.o: src/gui/main.cpp
	$(COMPILE) $(GTK_CFLAGS) -o out/guimain.o src/gui/main.cpp $(GTK_LIBS)

out/connection.o: src/connection/Connection.cpp
	$(COMPILE) -o out/connection.o -pthread src/connection/Connection.cpp

out/config.o: src/config/config.cpp
	$(COMPILE) -o out/config.o src/config/config.cpp

out/session.o: src/data/Session.cpp
	$(COMPILE) -o out/session.o src/data/Session.cpp

test:
	echo $(GTK_CFLAGS)

clean:
	rm out/main
	rm out/*.o
