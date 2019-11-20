CC=gcc
CFLAGS=-Wall -ggdb -O0 -std=gnu99 #Debug
#CFLAGS=-Wall -s -Os -std=gnu99 #Release
LDFLAGS=-pthread -lm

GLSL_CFLAGS=`pkg-config --cflags gl glfw3`
GLSL_LDFLAGS=`pkg-config --libs gl glfw3`

all: simple

simple: simple.c aux-GLSL.c
	$(CC) simple.c aux-GLSL.c -o simple $(CFLAGS) $(LDFLAGS) $(GLSL_CFLAGS) $(GLSL_LDFLAGS)

clean:
	rm -f simple

