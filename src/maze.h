#ifndef MAZE_H
#define MAZE_H

#include "raylib.h"

// Maze dimensions
#define MAZE_WIDTH      15
#define MAZE_HEIGHT     15

// Cell types
typedef enum {
    CELL_EMPTY = 0,     // Empty space
    CELL_WALL,          // Wall
    CELL_START,         // Start position
    CELL_EXIT,          // Exit position
    CELL_KEY,           // Key positions
    CELL_ENEMY          // Initial enemy positions
} CellType;

// Estructura del laberinto
typedef struct {
    unsigned char grid[MAZE_HEIGHT][MAZE_WIDTH];  // Maze grid (0=empty, 1=wall)
    Vector2 startPos;                           // Initial position
    Vector2 exitPos;                            // Final position
} Maze;

// Generate a new maze 
void GenerateMaze(Maze* maze);

// Obtain the type of cell at a given position
CellType GetCellType(Maze* maze, int x, int y);

// Verify if a position is within bounds
bool IsPosInBounds(int x, int y);

// Verify if a cell is walkable
bool IsCellWalkable(Maze* maze, int x, int y);

// Convert coordinates from world to grid
Vector2 WorldToGrid(Vector2 worldPos);

// Convert coordinates from grid to world
Vector2 GridToWorld(int gridX, int gridY);

#endif // MAZE_H