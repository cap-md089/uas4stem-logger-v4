COMPILE := g++ -g -c -Wall
LINK := g++ -Wall

#PKG_CONFIG_PATH:=$(PKG_CONFIG_PATH):/usr/lib/pkgconfig:/usr/share/pkgconfig:/lib/pkgconfig:/mingw64/lib/pkgconfig:/mingw64/share/pkgconfig
GTK_LIBS := -L/mingw64/lib -L/mingw64/lib/../lib -L/mingw64/lib -lgtk-3 -lgdk-3 -lgdi32 -limm32 -lshell32 -lole32 -Wl,-luuid -lwinmm -ldwmapi -lsetupapi -lcfgmgr32 -lz -lepoxy -lopengl32 -lgdi32 -lpangocairo-1.0 -lm -lgdi32 -lpangoft2-1.0 -lm -lpangowin32-1.0 -lm -lusp10 -lgdi32 -lpango-1.0 -lm -lfribidi -lthai -ldatrie -latk-1.0 -lcairo-gobject -lcairo -lz -lpixman-1 -lfontconfig -liconv -lexpat -lfreetype -lbz2 -lharfbuzz -lm -lusp10 -lgdi32 -lrpcrt4 -ldwrite -lgraphite2 -lpng16 -lz -lgdk_pixbuf-2.0 -lm -lgdiplus -lole32 -ljpeg -ljasper -lpng16 -lz -ltiff -lzstd -llzma -ljpeg -lz -lgio-2.0 -pthread -lintl -lshlwapi -ldnsapi -liphlpapi -lws2_32 -lgmodule-2.0 -pthread -lintl -lz -lgobject-2.0 -pthread -lintl -lffi -lglib-2.0 -lintl -pthread -lws2_32 -lwinmm -lws2_32 -lole32 -lwinmm -lshlwapi -lpcre
GTK_CFLAGS := -mms-bitfields -pthread -mms-bitfields -I/mingw64/include/gtk-3.0 -I/mingw64/include/cairo -I/mingw64/include -I/mingw64/include/pango-1.0 -I/mingw64/include/fribidi -I/mingw64/include -I/mingw64/include/atk-1.0 -I/mingw64/include/cairo -I/mingw64/include/pixman-1 -I/mingw64/include -I/mingw64/include/freetype2 -I/mingw64/include -I/mingw64/include/harfbuzz -I/mingw64/include -I/mingw64/include/libpng16 -I/mingw64/include/gdk-pixbuf-2.0 -I/mingw64/include/libpng16 -I/mingw64/include -I/mingw64/lib/libffi-3.2.1/include -I/mingw64/include/glib-2.0 -I/mingw64/lib/glib-2.0/include -I/mingw64/include
#GTK_LIBS := $(pkg-config --cflags --libs gtk+-3.0)
#GTK_LIBS := -I/usr/include/gtk-3.0 -I/usr/include/pango-1.0 -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include -I/usr/lib/libffi-3.2.1/include -I/usr/include/fribidi -I/usr/include/cairo -I/usr/include/pixman-1 -I/usr/include/freetype2 -I/usr/include/libpng16 -I/usr/include/harfbuzz -I/usr/include/uuid -I/usr/include/gdk-pixbuf-2.0 -I/usr/include/libmount -I/usr/include/blkid -I/usr/include/gio-unix-2.0 -I/usr/include/libdrm -I/usr/include/atk-1.0 -I/usr/include/at-spi2-atk/2.0 -I/usr/include/at-spi-2.0 -I/usr/include/dbus-1.0 -I/usr/lib/dbus-1.0/include -pthread -lgtk-3 -lgdk-3 -lpangocairo-1.0 -lpango-1.0 -latk-1.0 -lcairo-gobject -lcairo -lgdk_pixbuf-2.0 -lgio-2.0 -lgobject-2.0 -lglib-2.0

BASE_DIR := -I$(pwd)/src

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

clean:
	rm out/main
	rm out/*.o
