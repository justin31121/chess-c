#!/bin/sh -x

mkdir bin 2> /dev/null

gcc -o bin/single_player src/single_player.c
gcc -o bin/server src/server.c
gcc -o bin/client src/client.c
