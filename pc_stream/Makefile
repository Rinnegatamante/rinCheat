all: windows

windows:
	gcc -static-libgcc -o "rinCheat PSVITA streamer" main.c SDL_win32_main.c -static -L./ -lopengl32 -lSDL -lSDL_image -lSDL_mixer -DNO_STDIO_REDIRECT -lws2_32