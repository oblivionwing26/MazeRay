#include "maze.h"
#include <stdlib.h>
#include <time.h>
#include <string.h>

// Constants for maze generation
#define GRID_SCALE 1.0f   // Scale to convert grid coordinates to world coordinates

// Possible directions for the generation algorithm
typedef enum {
    DIR_NORTH = 0,
    DIR_EAST,
    DIR_SOUTH,
    DIR_WEST
} Direction;

// Offsets for each direction
const int DIRS[4][2] = {
    {0, -1},  // North
    {1, 0},   // East
    {0, 1},   // South
    {-1, 0}   // West
};

// Helper function to randomly shuffle a list of directions
static void ShuffleDirections(int dirs[4]) {
    for (int i = 0; i < 4; i++) {
        dirs[i] = i;
    }
    
    // Fisher-Yates algorithm for shuffling
    for (int i = 3; i > 0; i--) {
        int j = rand() % (i + 1);
        // Swap
        int temp = dirs[i];
        dirs[i] = dirs[j];
        dirs[j] = temp;
    }
}

// Recursive function to generate the maze using DFS
static void CarveMaze(Maze* maze, int x, int y) {
    int dirs[4];
    ShuffleDirections(dirs);
    
    for (int i = 0; i < 4; i++) {
        int nextX = x + DIRS[dirs[i]][0] * 2;
        int nextY = y + DIRS[dirs[i]][1] * 2;
        
        // Check if it's within bounds
        if (nextX > 0 && nextX < MAZE_WIDTH - 1 && nextY > 0 && nextY < MAZE_HEIGHT - 1) {
            // If the cell hasn't been visited (still a wall)
            if (maze->grid[nextY][nextX] == CELL_WALL) {
                // Open path
                maze->grid[y + DIRS[dirs[i]][1]][x + DIRS[dirs[i]][0]] = CELL_EMPTY;
                maze->grid[nextY][nextX] = CELL_EMPTY;
                
                // Continue carving from the new position
                CarveMaze(maze, nextX, nextY);
            }
        }
    }
}

// Place objects in the maze (keys, exit)
static void PlaceObjects(Maze* maze) {
    int keys = 0;
    int enemies = 0;
    int maxKeys = 3; // We could parameterize this in a more advanced version
    int maxEnemies = 5;
    
    // Find a point for the exit (far from the start)
    int exitX, exitY;
    float maxDist = 0;
    
    for (int y = 1; y < MAZE_HEIGHT - 1; y++) {
        for (int x = 1; x < MAZE_WIDTH - 1; x++) {
            if (maze->grid[y][x] == CELL_EMPTY) {
                // Calculate distance to start
                float dx = x - maze->startPos.x;
                float dy = y - maze->startPos.y;
                float dist = dx*dx + dy*dy;
                
                if (dist > maxDist) {
                    maxDist = dist;
                    exitX = x;
                    exitY = y;
                }
            }
        }
    }
    
    // Place exit
    maze->grid[exitY][exitX] = CELL_EXIT;
    maze->exitPos = (Vector2){ (float)exitX, (float)exitY };
    
    // Place keys randomly
    while (keys < maxKeys) {
        int x = rand() % (MAZE_WIDTH - 2) + 1;
        int y = rand() % (MAZE_HEIGHT - 2) + 1;
        
        if (maze->grid[y][x] == CELL_EMPTY) {
            // Make sure the key is not too close to the start or exit
            float dx1 = x - maze->startPos.x;
            float dy1 = y - maze->startPos.y;
            float dx2 = x - exitX;
            float dy2 = y - exitY;
            
            // Calculate squared distance (more efficient than square root)
            float distToStart = dx1*dx1 + dy1*dy1;
            float distToExit = dx2*dx2 + dy2*dy2;
            
            // Keys must be at a certain minimum distance
            if (distToStart > 5 && distToExit > 5) {
                maze->grid[y][x] = CELL_KEY;
                keys++;
            }
        }
    }
    
    // Place enemies
    while (enemies < maxEnemies) {
        int x = rand() % (MAZE_WIDTH - 2) + 1;
        int y = rand() % (MAZE_HEIGHT - 2) + 1;
        
        if (maze->grid[y][x] == CELL_EMPTY) {
            // Don't place enemies too close to the start
            float dx = x - maze->startPos.x;
            float dy = y - maze->startPos.y;
            float distToStart = dx*dx + dy*dy;
            
            if (distToStart > 9) { // Minimum distance
                maze->grid[y][x] = CELL_ENEMY;
                enemies++;
            }
        }
    }
}

// Generate a new random maze
void GenerateMaze(Maze* maze) {
    // Initialize everything as walls
    memset(maze->grid, CELL_WALL, sizeof(maze->grid));
    
    // Choose random starting point (must be odd)
    int startX = 1;
    int startY = 1;
    
    // Mark start as empty space
    maze->grid[startY][startX] = CELL_EMPTY;
    maze->startPos = (Vector2){ (float)startX, (float)startY };
    
    // Start recursive generation
    CarveMaze(maze, startX, startY);
    
    // Place objects
    PlaceObjects(maze);
}

// Get cell type at a specific position
CellType GetCellType(Maze* maze, int x, int y) {
    // Check bounds
    if (x < 0 || x >= MAZE_WIDTH || y < 0 || y >= MAZE_HEIGHT) {
        return CELL_WALL; // Out of bounds is considered a wall
    }
    
    return (CellType)maze->grid[y][x];
}

// Check if a position is within the maze bounds
bool IsPosInBounds(int x, int y) {
    return x >= 0 && x < MAZE_WIDTH && y >= 0 && y < MAZE_HEIGHT;
}

// Check if a position is walkable (not a wall)
bool IsCellWalkable(Maze* maze, int x, int y) {
    if (!IsPosInBounds(x, y)) return false;
    
    CellType cellType = GetCellType(maze, x, y);
    return cellType != CELL_WALL;
}

// Convert world coordinates to grid coordinates
Vector2 WorldToGrid(Vector2 worldPos) {
    return (Vector2){
        worldPos.x / GRID_SCALE,
        worldPos.y / GRID_SCALE
    };
}

// Convert grid coordinates to world coordinates
Vector2 GridToWorld(int gridX, int gridY) {
    return (Vector2){
        gridX * GRID_SCALE + GRID_SCALE/2,  // Center of the cell
        gridY * GRID_SCALE + GRID_SCALE/2
    };
}