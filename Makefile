
CFLAGS = -Wall -Wextra -std=c99
INCLUDES = -I./include
GCCLDFLAGS = -Llibs/raylib/linux
GCCLDLIBS = -l:libraylib.a -lm -lGL -lpthread -ldl -lrt -lX11
MINGWLDFLAGS = -Llibs/raylib/win32-mingw
MINGWLDLIBS = -lraylib -lwinmm -lgdi32 -lopengl32 -lmingwex

.PHONY: all linux mingw

linux: always
	gcc $(CFLAGS) -o build/midiorite $(INCLUDES) main.c $(GCCLDFLAGS) $(GCCLDLIBS)

mingw:
	x86_64-w64-mingw32-gcc $(CFLAGS) -o build/midiorite.exe $(INCLUDES) main.c \
	-Wl,--defsym,stat64i32=_stat64 -Wl,--subsystem,windows \
	$(MINGWLDFLAGS) $(MINGWLDLIBS)

always:
	mkdir -p build

