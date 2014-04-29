all: healpixviewer

healpixviewer: main.cpp
	g++ -Wno-deprecated -Wall $(shell pkg-config --cflags --libs chealpix glfw3) -framework OpenGL $< -o $@

clean:
	rm -f healpixviewer