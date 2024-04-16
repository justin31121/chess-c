mkdir bin 2> nul

gcc -o bin\single_player src\single_player.c
gcc -o bin\server src\server.c -lws2_32
gcc -o bin\client src\client.c -lws2_32
