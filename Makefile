all: healpixviewer

healpixviewer: main.c
	gcc -Wno-deprecated -Wall $(shell pkg-config --cflags --libs chealpix glfw3) -framework OpenGL $< -o $@

clean:
	rm -f healpixviewer