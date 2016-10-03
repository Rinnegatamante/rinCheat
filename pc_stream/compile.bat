windres profile.rc -O coff -o icon.res
gcc SDL_win32_main.c main.c icon.res -o "rinCheat PSVITA streamer" -lopengl32 -lSDL -lSDL_image -DNO_STDIO_REDIRECT -lws2_32