mkdir bin 2> nul

cl /Fe:bin\single_player src\single_player.c
cl /Fe:bin\server src\server.c ws2_32.lib
cl /Fe:bin\client src\client.c ws2_32.lib
cl /Fe:bin\single_player_ui src\single_player_ui.c gdi32.lib user32.lib opengl32.lib
