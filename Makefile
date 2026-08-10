
CFLAGS = -Wall -Wextra
INCLUDES = -Iinclude
LDFLAGS = -Llibs
LDLIBS = -lraylib -lm -lGL -lpthread -ldl -lrt -lX11

midi-orite:
	gcc $(CFLAGS) -o build/midi-orite $(INCLUDES) main.c $(LDFLAGS) $(LDLIBS)
