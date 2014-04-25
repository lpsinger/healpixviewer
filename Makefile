all: healpixviewer

healpixviewer: main.c
	gcc -Wno-deprecated $(shell pkg-config --cflags --libs chealpix) -framework OpenGL -framework GLUT $< -o $@

clean:
	rm -f healpixviewer