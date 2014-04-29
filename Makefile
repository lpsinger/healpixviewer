all: healpixviewer

healpixviewer: main.c
	gcc -g -Wno-deprecated $(shell pkg-config --cflags --libs chealpix glfw3) -framework OpenGL $< -o $@

clean:
	rm -f healpixviewer