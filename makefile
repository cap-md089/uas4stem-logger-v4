COMPILE := g++ -g -c -Wall
LINK := g++ -Wall

PKG_CONFIG_PATH:=$(PKG_CONFIG_PATH):/usr/lib/pkgconfig:/usr/share/pkgconfig:/lib/pkgconfig:/mingw64/lib/pkgconfig:/mingw64/share/pkgconfig
GTK_LIBS := -mms-bitfields -pthread -mms-bitfields -I/mingw64/include/gtk-3.0 -I/mingw64/include/cairo -I/mingw64/include -I/mingw64/include/pango-1.0 -I/mingw64/include/fribidi -I/mingw64/include -I/mingw64/include/atk-1.0 -I/mingw64/include/cairo -I/mingw64/include/pixman-1 -I/mingw64/include -I/mingw64/include/freetype2 -I/mingw64/include -I/mingw64/include/harfbuzz -I/mingw64/include -I/mingw64/include/libpng16 -I/mingw64/include/gdk-pixbuf-2.0 -I/mingw64/include/libpng16 -I/mingw64/include -I/mingw64/lib/libffi-3.2.1/include -I/mingw64/include/glib-2.0 -I/mingw64/lib/glib-2.0/include -I/mingw64/include -L/mingw64/lib -L/mingw64/lib/../lib -L/mingw64/lib -lgtk-3 -lgdk-3 -lgdi32 -limm32 -lshell32 -lole32 -Wl,-luuid -lwinmm -ldwmapi -lsetupapi -lcfgmgr32 -lz -lepoxy -lopengl32 -lgdi32 -lpangocairo-1.0 -lm -lgdi32 -lpangoft2-1.0 -lm -lpangowin32-1.0 -lm -lusp10 -lgdi32 -lpango-1.0 -lm -lfribidi -lthai -ldatrie -latk-1.0 -lcairo-gobject -lcairo -lz -lpixman-1 -lfontconfig -liconv -lexpat -lfreetype -lbz2 -lharfbuzz -lm -lusp10 -lgdi32 -lrpcrt4 -ldwrite -lgraphite2 -lpng16 -lz -lgdk_pixbuf-2.0 -lm -lgdiplus -lole32 -ljpeg -ljasper -lpng16 -lz -ltiff -lzstd -llzma -ljpeg -lz -lgio-2.0 -pthread -lintl -lshlwapi -ldnsapi -liphlpapi -lws2_32 -lgmodule-2.0 -pthread -lintl -lz -lgobject-2.0 -pthread -lintl -lffi -lglib-2.0 -lintl -pthread -lws2_32 -lwinmm -lws2_32 -lole32 -lwinmm -lshlwapi -lpcre
# GTKLIBS := $(pkg-config --libs --cflags gtk+-3.0)

out/main: out/CurrentState.o out/math.o out/main.o
	$(LINK) -o out/main out/CurrentStateTests.o out/math.o out/main.o $(GTK_LIBS) 

out/math.o: src/math/Vector3D.cpp
	$(COMPILE) -o out/math.o src/math/Vector3D.cpp

out/main.o: src/main.cpp
	$(COMPILE) -o out/main.o src/main.cpp $(GTK_LIBS)

out/CurrentState.o: src/CurrentState.cpp
	$(COMPILE) -o out/CurrentState.o src/CurrentState.cpp

clean:
	rm out/main
	rm out/*.o
