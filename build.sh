#!/bin/sh -x

mkdir bin 2> /dev/null

gcc -I../js-c -o bin/single_player src/single_player.c
gcc -I../js-c -o bin/server src/server.c
gcc -I../js-c -o bin/client src/client.c
gcc -I../js-c -o bin/single_player_ui src/single_player_ui.c -lGLX -lX11 -lm -lGL
gcc -I../js-c -o bin/client_ui src/client_ui.c -lGLX -lX11 -lm -lGL
