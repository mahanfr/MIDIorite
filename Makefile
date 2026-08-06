
CFLAGS = -Wall -Wextera
INCLUDES = -Iinclude
LDFLAGS = -Llibs
LDLIBS = -lraylib -lm -lGL -lpthread -ldl -lrt -lX11

midi-orite:
	gcc -o build/midi-orite $(INCLUDES) main.c $(LDFLAGS) $(LDLIBS)
