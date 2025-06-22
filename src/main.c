/*******************************************************************************************
*
*   MazeRay - Un shooter estilo Wolfenstein 3D que cabe en un disquete de 1.44MB
*
*   Este juego es parte de un desafío para crear un juego completo que quepa en un disquete
*
*******************************************************************************************/

#include "raylib.h"
#include "game.h"
#include "maze.h"
#include "utils.h"

#define WINDOW_TITLE     "MazeRay v1.0"

// Initial configuration 
#define SCREEN_WIDTH     1280
#define SCREEN_HEIGHT    720
#define FULLSCREEN       1

int main(void) {
    // Init window and audio device
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, WINDOW_TITLE);
    InitAudioDevice();
    
    SetTargetFPS(60);
    
    #if FULLSCREEN
    ToggleFullscreen();
    #endif
    
    // Initialize game variables
    InitGame();
    
    // Main game loop
    while (!WindowShouldClose()) {  
        UpdateGame();               
        RenderGame();                
    }
    
    // Free resources
    CloseGame();
    CloseWindow();
    
    return 0;
}