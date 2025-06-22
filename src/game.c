#include "game.h"
#include "utils.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// ----------------------------------------------------------------------------------
// Global Variables
// ----------------------------------------------------------------------------------
static GameState currentState;        // Current game state
static Player player;                 // Player data
static Enemy enemies[MAX_ENEMIES];    // Array of enemies
static Maze gameMaze;                 // Maze structure
static int enemyCount;                // Current number of enemies
static Sprite sprites[MAX_SPRITES];   // Array of sprites (keys, enemies, etc.)
static int spriteCount;               // Current number of sprites
static bool showExitMessage = false;  // Show victory message
static float exitMessageTimer = 0.0f; // Timer for victory message
static WeaponAnimation katanaAnim;

// Textures
static Texture2D wallTextures[1];     // Wall texture (only one to save space)
static Texture2D weaponTexture;       // Player's weapon texture
static Texture2D keyTexture;          // Key texture
static Texture2D impTextures[56];     // Array of individual enemy textures

// Sounds
static Sound footstepSound;
static Sound victorySound;
static Sound keyPickupSound;
static Sound playerHitSound;          // Sound when player takes damage
static Sound gameOverSound;           // Game over sound
static Sound shootSound;

// Raycasting engine variables
static float projPlaneDistance;       // Distance to projection plane
static int numRays;                   // Number of rays for raycasting
static const float raycastMaxDistance = 20.0f; // Maximum raycast distance
static void CheckPlayerInteractions(void);

// Definitions for enemy animations
#define ANIM_WALK_FRONT          0  // 000-003: Walk from front
#define ANIM_ATTACK_FRONT        1  // 004-007: Attack from front
#define ANIM_WALK_DIAG_FRONT_L   2  // 008-011: Walk from front left diagonal
#define ANIM_ATTACK_DIAG_FRONT_L 3  // 012-015: Attack from front left diagonal
#define ANIM_WALK_SIDE           4  // 016-019: Walk from side view
#define ANIM_ATTACK_SIDE         5  // 020-023: Attack from side view
#define ANIM_WALK_DIAG_BACK_L    6  // 024-027: Walk from back left diagonal
#define ANIM_ATTACK_DIAG_BACK_L  7  // 028-031: Attack from back left diagonal view
#define ANIM_WALK_BACK           8  // 032-035: Walk from back view
#define ANIM_ATTACK_BACK         9  // 036-039: Attack from back view
#define ANIM_ATTACK_DIAG_FRONT_R 10 // 040-042: Attack from front right diagonal
#define ANIM_DEATH               11 // 049-055: Death

// Animation to sprite range mapping
typedef struct {
    int startFrame;
    int endFrame;
    bool loop;
} AnimationRange;

static const AnimationRange animationRanges[] = {
    { 0, 3, true },    // ANIM_WALK_FRONT
    { 4, 7, true },    // ANIM_ATTACK_FRONT
    { 8, 11, true },   // ANIM_WALK_DIAG_FRONT_L
    { 12, 15, true },  // ANIM_ATTACK_DIAG_FRONT_L
    { 16, 19, true },  // ANIM_WALK_SIDE
    { 20, 23, true },  // ANIM_ATTACK_SIDE
    { 24, 27, true },  // ANIM_WALK_DIAG_BACK_L
    { 28, 31, true },  // ANIM_ATTACK_DIAG_BACK_L
    { 32, 35, true },  // ANIM_WALK_BACK
    { 36, 39, true },  // ANIM_ATTACK_BACK
    { 40, 42, true },  // ANIM_ATTACK_DIAG_FRONT_R
    { 49, 55, false }  // ANIM_DEATH
};

// ----------------------------------------------------------------------------------
// Local Functions (private)
// ----------------------------------------------------------------------------------

// Initialize player
static void InitPlayer(void) {
    player.position = GridToWorld((int)gameMaze.startPos.x, (int)gameMaze.startPos.y);
    player.angle = 0.0f;
    player.health = 100;
    player.keys = 0;
    player.shootCooldown = 0.0f;

    katanaAnim.isSwinging = false;
    katanaAnim.swingTimer = 0.0f;
    katanaAnim.swingDuration = 0.5f; // Attack duration
    katanaAnim.basePosition = (Vector2){GetScreenWidth() - 250, GetScreenHeight() - 200};
    katanaAnim.swingOffset = (Vector2){-30.0f, -20.0f};
    katanaAnim.scale = 7.0f; // Increase scale for better visibility
    katanaAnim.currentFrame = 0;
    katanaAnim.frameTimer = 0.0f;
    
    crosshair.position = (Vector2){ GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f };
    crosshair.size = 15.0f;
    crosshair.color = WHITE;
    
    // Load weapon animation frames
    for (int i = 0; i < 5; i++) {
        char filename[100];
        sprintf(filename, "assets/textures/weapons/tile%03d.png", i + 6);
        katanaAnim.frames[i] = LoadTexture(filename);
        
        // Verify if texture loaded correctly
        if (katanaAnim.frames[i].id == 0) {
            printf("Error loading texture: %s\n", filename);
            
            // Try loading from alternative path
            sprintf(filename, "assets/textures/weaponsAttachment/tile%03d.png", i + 6);
            katanaAnim.frames[i] = LoadTexture(filename);
            
            if (katanaAnim.frames[i].id == 0) {
                printf("Error loading alternative texture: %s\n", filename);
            } else {
                printf("Alternative texture loaded successfully: %s\n", filename);
            }
        } else {
            printf("Texture loaded successfully: %s, ID: %u\n", filename, katanaAnim.frames[i].id);
        }
    }
}

// Load enemy textures
static void LoadEnemyTextures(void) {
    char filename[64];
    for (int i = 0; i < 56; i++) {
        sprintf(filename, "assets/textures/imp/tile%03d.png", i);
        impTextures[i] = LoadTexture(filename);
    }
}

// Get texture index based on animation and current frame
static int GetTextureIndex(int animationType, int currentFrame) {
    if (animationType < 0 || animationType >= sizeof(animationRanges)/sizeof(AnimationRange)) {
        return 0; // Default value if animation is invalid
    }
    
    int startFrame = animationRanges[animationType].startFrame;
    int endFrame = animationRanges[animationType].endFrame;
    int frameCount = endFrame - startFrame + 1;
    
    // Ensure frame is within range
    int frameIndex = currentFrame % frameCount;
    
    return startFrame + frameIndex;
}

// Initialize enemies
static void InitEnemies(void) {
    enemyCount = 0;
    spriteCount = 0;
    
     // Load key sprites first
     for (int y = 0; y < MAZE_HEIGHT; y++) {
        for (int x = 0; x < MAZE_WIDTH; x++) {
            if (GetCellType(&gameMaze, x, y) == CELL_KEY && spriteCount < MAX_SPRITES) {
                sprites[spriteCount].position = GridToWorld(x, y);
                sprites[spriteCount].texture = keyTexture;
                sprites[spriteCount].active = true;
                sprites[spriteCount].type = CELL_KEY;
                // For keys, we use a simple frame that covers the entire texture
                sprites[spriteCount].frame = (Rectangle){ 0, 0, keyTexture.width, keyTexture.height };
                
                // Debug to verify key textures
                printf("Key sprite added. Texture ID: %u, Width: %d, Height: %d\n", 
                      keyTexture.id, keyTexture.width, keyTexture.height);
                
                spriteCount++;
            }
        }
    }
    
    // Add the sprite of the exit door
    sprites[spriteCount].position = exitDoor.position;
    sprites[spriteCount].texture = exitDoor.isOpen ? exitDoor.openTexture : exitDoor.closedTexture;
    sprites[spriteCount].active = true;
    sprites[spriteCount].type = CELL_EXIT;
    sprites[spriteCount].frame = (Rectangle){ 
        0, 0, 
        sprites[spriteCount].texture.width, 
        sprites[spriteCount].texture.height 
    };
    exitDoor.spriteIndex = spriteCount; // Store the index of the door sprite
    spriteCount++;
    
    // Search the maze for enemy positions
    for (int y = 0; y < MAZE_HEIGHT; y++) {
        for (int x = 0; x < MAZE_WIDTH; x++) {
            if (GetCellType(&gameMaze, x, y) == CELL_ENEMY && enemyCount < MAX_ENEMIES) {
                enemies[enemyCount].position = GridToWorld(x, y);
                enemies[enemyCount].angle = ((float)rand() / RAND_MAX) * 2 * PI;
                enemies[enemyCount].active = true;
                enemies[enemyCount].health = ENEMY_HEALTH;
                enemies[enemyCount].attackCooldown = 0.0f;
                enemies[enemyCount].moveTimer = ((float)rand() / RAND_MAX) * 2.0f;
                enemies[enemyCount].direction = (Vector2){
                    cosf(enemies[enemyCount].angle),
                    sinf(enemies[enemyCount].angle)
                };
                
                // Setup animation
                enemies[enemyCount].anim.currentAnim = ANIM_WALK_FRONT;
                enemies[enemyCount].anim.currentFrame = 0;
                enemies[enemyCount].anim.frameTimer = 0.0f;
                enemies[enemyCount].anim.isPlaying = true;
                enemies[enemyCount].anim.loop = true;
                enemies[enemyCount].isDying = false;
                
                // Add sprite for the enemy
                if (spriteCount < MAX_SPRITES) {
                    sprites[spriteCount].position = enemies[enemyCount].position;
                    sprites[spriteCount].texture = impTextures[0]; // Initial texture
                    sprites[spriteCount].active = true;
                    sprites[spriteCount].type = CELL_ENEMY;
                    
                    // Setup initial frame
                    sprites[spriteCount].frame = (Rectangle){ 0, 0, impTextures[0].width, impTextures[0].height };
                    sprites[spriteCount].anim = enemies[enemyCount].anim;
                    
                    // Save reference to sprite in the enemy
                    enemies[enemyCount].spriteIndex = spriteCount;
                    
                    spriteCount++;
                }
                
                enemyCount++;
            }
        }
    }
}

// Check player collisions with the world
static bool CheckWallCollision(Vector2 pos) {
    // Convert world position to grid coordinates
    Vector2 gridPos = WorldToGrid(pos);
    int gridX = (int)gridPos.x;
    int gridY = (int)gridPos.y;
    
    // Check if cell is a wall
    return !IsCellWalkable(&gameMaze, gridX, gridY);
}

// Move player
static void MovePlayer(void) {
    float moveSpeed = PLAYER_SPEED * GetFrameTime();
    float rotSpeed = PLAYER_ROT_SPEED * GetFrameTime();

    // Rotate player
    if (IsKeyDown(KEY_LEFT)) player.angle -= rotSpeed;
    if (IsKeyDown(KEY_A)) player.angle -= rotSpeed;
    if (IsKeyDown(KEY_D)) player.angle += rotSpeed;
    if (IsKeyDown(KEY_RIGHT)) player.angle += rotSpeed;
    
    // Normalize angle
    player.angle = NormalizeAngle(player.angle);
    
    // Calculate direction vector
    float dirX = cosf(player.angle);
    float dirY = sinf(player.angle);
    
    // Move forward/backward
    Vector2 newPos = player.position;
    
    if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) {
        newPos.x += dirX * moveSpeed;
        newPos.y += dirY * moveSpeed;
        if (!IsSoundPlaying(footstepSound)) {
            PlaySound(footstepSound);
        }
    }
    
    if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) {
        newPos.x -= dirX * moveSpeed;
        newPos.y -= dirY * moveSpeed;
        if (!IsSoundPlaying(footstepSound)) {
            PlaySound(footstepSound);
        }
    }
    
    // Check collisions separately in X and Y to allow wall sliding
    Vector2 testPos = player.position;
    testPos.x = newPos.x;
    
    if (!CheckWallCollision(testPos)) {
        player.position.x = testPos.x;
    }
    
    testPos = player.position;
    testPos.y = newPos.y;
    
    if (!CheckWallCollision(testPos)) {
        player.position.y = testPos.y;
    }
    
    // Check interactions with objects
    CheckPlayerInteractions();
}

// Determine the enemy animation based on its state and relative angle to the player
static int DetermineEnemyAnimation(Enemy* enemy, bool isAttacking, bool isDying) {
    if (isDying) {
        return ANIM_DEATH;
    }
    
    if (isAttacking) {
        // Calculate relative angle between player and enemy
        float dx = player.position.x - enemy->position.x;
        float dy = player.position.y - enemy->position.y;
        float targetAngle = atan2f(dy, dx);
        float angleDiff = NormalizeAngle(player.angle - targetAngle);
        
        // Convert angle difference to degrees for easier comparisons
        float angleDegrees = RadToDeg(angleDiff);
        
        // Normalize to 0-360
        while (angleDegrees < 0) angleDegrees += 360.0f;
        while (angleDegrees >= 360.0f) angleDegrees -= 360.0f;
        
        // Select attack animation based on angle
        if (angleDegrees >= 315 || angleDegrees < 45) {
            return ANIM_ATTACK_FRONT;
        } else if (angleDegrees >= 45 && angleDegrees < 90) {
            return ANIM_ATTACK_DIAG_FRONT_L;
        } else if (angleDegrees >= 90 && angleDegrees < 135) {
            return ANIM_ATTACK_SIDE;
        } else if (angleDegrees >= 135 && angleDegrees < 225) {
            return ANIM_ATTACK_BACK;
        } else if (angleDegrees >= 225 && angleDegrees < 270) {
            return ANIM_ATTACK_DIAG_BACK_L;
        } else {
            return ANIM_ATTACK_DIAG_FRONT_R;
        }
    } else {
        // Calculate relative angle for walking animation
        float dx = player.position.x - enemy->position.x;
        float dy = player.position.y - enemy->position.y;
        float targetAngle = atan2f(dy, dx);
        float angleDiff = NormalizeAngle(player.angle - targetAngle);
        
        // Convert angle difference to degrees
        float angleDegrees = RadToDeg(angleDiff);
        
        // Normalize to 0-360
        while (angleDegrees < 0) angleDegrees += 360.0f;
        while (angleDegrees >= 360.0f) angleDegrees -= 360.0f;
        
        // Select walking animation based on angle
        if (angleDegrees >= 315 || angleDegrees < 45) {
            return ANIM_WALK_FRONT;
        } else if (angleDegrees >= 45 && angleDegrees < 90) {
            return ANIM_WALK_DIAG_FRONT_L;
        } else if (angleDegrees >= 90 && angleDegrees < 135) {
            return ANIM_WALK_SIDE;
        } else if (angleDegrees >= 135 && angleDegrees < 225) {
            return ANIM_WALK_BACK;
        } else {
            return ANIM_WALK_DIAG_BACK_L;
        }
    }
}

// Update enemies
static void UpdateEnemies(void) {
    float deltaTime = GetFrameTime();
    
    for (int i = 0; i < enemyCount; i++) {
        if (!enemies[i].active) continue;
        
        // Update attack timer
        if (enemies[i].attackCooldown > 0) {
            enemies[i].attackCooldown -= deltaTime;
        }
        
        // Update animation
        enemies[i].anim.frameTimer += deltaTime;
        
        // Determine which animation to use based on state
        float distToPlayer = Distance(
            enemies[i].position.x, enemies[i].position.y,
            player.position.x, player.position.y
        );
        
        bool isAttacking = (distToPlayer <= ENEMY_ATTACK_RANGE);
        int newAnimType = DetermineEnemyAnimation(&enemies[i], isAttacking, enemies[i].isDying);
        
        // If animation changed, reset
        if (enemies[i].anim.currentAnim != newAnimType) {
            enemies[i].anim.currentAnim = newAnimType;
            enemies[i].anim.currentFrame = 0;
            enemies[i].anim.frameTimer = 0.0f;
            enemies[i].anim.loop = animationRanges[newAnimType].loop;
        }
        
        // Change frame if it's time
        if (enemies[i].anim.frameTimer >= ENEMY_FRAME_TIME) {
            enemies[i].anim.frameTimer = 0.0f;
            
            // Advance to next frame
            enemies[i].anim.currentFrame++;
            
            // Get frame range for this animation
            int startFrame = animationRanges[enemies[i].anim.currentAnim].startFrame;
            int endFrame = animationRanges[enemies[i].anim.currentAnim].endFrame;
            int frameCount = endFrame - startFrame + 1;
            
            // If we reached the end of animation
            if (enemies[i].anim.currentFrame >= frameCount) {
                if (enemies[i].anim.loop) {
                    enemies[i].anim.currentFrame = 0; // Return to start
                } else {
                    enemies[i].anim.currentFrame = frameCount - 1; // Stay on last frame
                    
                    // If it's death animation and finished
                    if (enemies[i].anim.currentAnim == ANIM_DEATH) {
                        enemies[i].active = false;
                    }
                }
            }
        }
        
        // Update enemy angle to face player
        float dx = player.position.x - enemies[i].position.x;
        float dy = player.position.y - enemies[i].position.y;
        float targetAngle = atan2f(dy, dx);
        enemies[i].angle = targetAngle;
        
        // Update enemy sprite
        int spriteIndex = enemies[i].spriteIndex;
        if (spriteIndex >= 0 && spriteIndex < spriteCount) {
            // Get correct texture index
            int textureIndex = GetTextureIndex(
                enemies[i].anim.currentAnim, 
                enemies[i].anim.currentFrame
            );
            
            // Update sprite texture
            sprites[spriteIndex].texture = impTextures[textureIndex];
            
            // Update frame to cover entire texture
            sprites[spriteIndex].frame = (Rectangle){
                0, 0, 
                (float)impTextures[textureIndex].width, 
                (float)impTextures[textureIndex].height
            };
            
            // Update sprite position with enemy position
            sprites[spriteIndex].position = enemies[i].position;
            
            // Also update sprite animation
            sprites[spriteIndex].anim = enemies[i].anim;
        }
        
        // If enemy is in attack range
        if (distToPlayer <= ENEMY_ATTACK_RANGE) {
            // Attack player if cooldown allows
            if (enemies[i].attackCooldown <= 0) {
                player.health -= ENEMY_ATTACK_DAMAGE;
                enemies[i].attackCooldown = ENEMY_ATTACK_COOLDOWN;
                
                // Play damage sound
                PlaySound(playerHitSound);
                
                // Check if player has died
                if (player.health <= 0) {
                    player.health = 0;
                    currentState = GAME_OVER;
                    PlaySound(gameOverSound);
                }
            }
        }
        // If not in attack range, move
        else {
            // Update movement timer
            enemies[i].moveTimer -= deltaTime;
            
            // Change direction randomly
            if (enemies[i].moveTimer <= 0) {
                // Choose between following player or moving randomly
                if (distToPlayer < 5.0f && ((float)rand() / RAND_MAX) < 0.7f) {
                    // Follow player
                    float dx = player.position.x - enemies[i].position.x;
                    float dy = player.position.y - enemies[i].position.y;
                    float length = sqrtf(dx*dx + dy*dy);
                    
                    if (length > 0) {
                        enemies[i].direction.x = dx / length;
                        enemies[i].direction.y = dy / length;
                    }
                } else {
                    // Move randomly
                    float randomAngle = ((float)rand() / RAND_MAX) * 2 * PI;
                    enemies[i].direction.x = cosf(randomAngle);
                    enemies[i].direction.y = sinf(randomAngle);
                }
                
                // Reset timer (between 1 and 3 seconds)
                enemies[i].moveTimer = 1.0f + ((float)rand() / RAND_MAX) * 2.0f;
            }
            
            // Calculate new position
            Vector2 newPos = enemies[i].position;
            newPos.x += enemies[i].direction.x * ENEMY_SPEED * deltaTime;
            newPos.y += enemies[i].direction.y * ENEMY_SPEED * deltaTime;
            
            // Check collisions separately in X and Y
            Vector2 testPos = enemies[i].position;
            testPos.x = newPos.x;
            
            if (!CheckWallCollision(testPos)) {
                enemies[i].position.x = testPos.x;
            } else {
                // If collision, invert X direction
                enemies[i].direction.x *= -1;
            }
            
            testPos = enemies[i].position;
            testPos.y = newPos.y;
            
            if (!CheckWallCollision(testPos)) {
                enemies[i].position.y = testPos.y;
            } else {
                // If collision, invert Y direction
                enemies[i].direction.y *= -1;
            }
        }
    }
}

// Check player attacks against enemies
static void CheckPlayerAttacks(void) {
    // Only check if the player is attacking
    if (!katanaAnim.isSwinging) return;
    
    // Play shooting sound
    PlaySound(shootSound);
    
    // Get the ray direction from the camera
    float playerDirX = cosf(player.angle);
    float playerDirY = sinf(player.angle);
    
    // Check each enemy
    for (int i = 0; i < enemyCount; i++) {
        if (!enemies[i].active || enemies[i].isDying) continue;
        
        // Calculate vector from player to enemy
        float dx = enemies[i].position.x - player.position.x;
        float dy = enemies[i].position.y - player.position.y;
        
        // Calculate distance to enemy
        float dist = sqrtf(dx*dx + dy*dy);
        
        // Normalize the direction vector to the enemy
        if (dist > 0) {
            dx /= dist;
            dy /= dist;
        }
        
        // Calculate dot product to check if enemy is in front of player
        float dotProduct = dx * playerDirX + dy * playerDirY;
        
        // If the enemy is in front of the player (within a vision cone)
        if (dotProduct > 0.9f) { // Approximately ±25 degrees
            // Cast a ray from the player in the enemy's direction
            float rayDist = 0.0f;
            bool hitWall = false;
            float stepSize = 0.1f;
            
            while (rayDist < dist && !hitWall) {
                // Move along the ray
                float rayX = player.position.x + playerDirX * rayDist;
                float rayY = player.position.y + playerDirY * rayDist;
                
                // Check if we hit a wall
                Vector2 rayPos = (Vector2){ rayX, rayY };
                if (CheckWallCollision(rayPos)) {
                    hitWall = true;
                }
                
                rayDist += stepSize;
            }
            
            // If we didn't hit a wall before reaching the enemy, damage it
            if (!hitWall) {
                enemies[i].health -= PLAYER_ATTACK_DAMAGE;
                
                
                // Check if the enemy has died
                if (enemies[i].health <= 0) {
                    enemies[i].isDying = true;
                    enemies[i].anim.currentAnim = ANIM_DEATH;
                    enemies[i].anim.currentFrame = 0;
                    enemies[i].anim.loop = false;
                }
            }
        }
    }
}

// Check player interactions with objects
static void CheckPlayerInteractions(void) {
    // Update door state based on keys
    bool oldState = exitDoor.isOpen;
    exitDoor.isOpen = (player.keys >= MAX_KEYS);
    
    // Only update the texture if the state has changed
    if (oldState != exitDoor.isOpen && exitDoor.spriteIndex >= 0 && exitDoor.spriteIndex < spriteCount) {
        sprites[exitDoor.spriteIndex].texture = exitDoor.isOpen ? 
            exitDoor.openTexture : exitDoor.closedTexture;
        
        // Debug print
        printf("Door state changed to: %s\n", exitDoor.isOpen ? "OPEN" : "CLOSED");
        printf("Door sprite texture ID: %u\n", sprites[exitDoor.spriteIndex].texture.id);
        
        sprites[exitDoor.spriteIndex].frame = (Rectangle){ 
            0, 0, 
            sprites[exitDoor.spriteIndex].texture.width, 
            sprites[exitDoor.spriteIndex].texture.height 
        };
    }
    
    // Check interactions with sprites (keys and exit door)
    for (int i = 0; i < spriteCount; i++) {
        if (!sprites[i].active) continue;
        
        // Calculate distance to sprite
        float dist = Distance(player.position.x, player.position.y, 
                             sprites[i].position.x, sprites[i].position.y);
        
        // If we're close enough
        if (dist < 0.5f) {
            // If it's a key
            if (sprites[i].type == CELL_KEY) {
                player.keys++;
                sprites[i].active = false;
                
                // Also remove the key from grid to avoid duplicates
                Vector2 spriteGridPos = WorldToGrid(sprites[i].position);
                int spriteGridX = (int)spriteGridPos.x;
                int spriteGridY = (int)spriteGridPos.y;
                
                if (GetCellType(&gameMaze, spriteGridX, spriteGridY) == CELL_KEY) {
                    gameMaze.grid[spriteGridY][spriteGridX] = CELL_EMPTY;
                }
                
                PlaySound(keyPickupSound);
            }
            // If it's the exit door and it's open
            else if (sprites[i].type == CELL_EXIT && exitDoor.isOpen) {
                currentState = GAME_VICTORY;
                PlaySound(victorySound);
                exitMessageTimer = 3.0f; // Show message for 3 seconds
            }
        }
    }
    
    // Show message if trying to exit without enough keys
    Vector2 gridPos = WorldToGrid(player.position);
    int gridX = (int)gridPos.x;
    int gridY = (int)gridPos.y;
    
    if (GetCellType(&gameMaze, gridX, gridY) == CELL_EXIT && !exitDoor.isOpen) {
        showExitMessage = true;
        exitMessageTimer = 2.0f;
    }

    if (showExitMessage) {
        exitMessageTimer -= GetFrameTime();
        if (exitMessageTimer <= 0.0f) {
            showExitMessage = false;
        }
    }
}
// Raycasting engine for 3D rendering
static void RenderRaycasting(float zBuffer[]) {
    float fovHalf = DegToRad(FOV / 2);
    float screenWidth = (float)GetScreenWidth();
    float screenHeight = (float)GetScreenHeight();
    
    // Calculate camera direction and plane
    float playerDirX = cosf(player.angle);
    float playerDirY = sinf(player.angle);
    float planeX = -playerDirY * tanf(fovHalf);
    float planeY = playerDirX * tanf(fovHalf);
    
    ClearBackground(BLACK);
    
    // Draw floor/ceiling
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight()/2, (Color){25, 25, 25, 255});                 // Ceiling
    DrawRectangle(0, GetScreenHeight()/2, GetScreenWidth(), GetScreenHeight()/2, (Color){50, 50, 50, 255}); // Floor
    
    // Cast rays
    for (int x = 0; x < screenWidth; x++) {
        // Calculate x-position in camera space
        float cameraX = 2.0f * x / screenWidth - 1.0f;
        
        // Calculate ray direction
        float rayDirX = playerDirX + planeX * cameraX;
        float rayDirY = playerDirY + planeY * cameraX;
        
        // Current map position
        int mapX = (int)player.position.x;
        int mapY = (int)player.position.y;
        
        // Length of ray from one side to next
        float deltaDistX = fabsf(rayDirX) < 0.00001f ? 1e30f : fabsf(1.0f / rayDirX);
        float deltaDistY = fabsf(rayDirY) < 0.00001f ? 1e30f : fabsf(1.0f / rayDirY);
        
        // Calculate step and initial side distance
        int stepX, stepY;
        float sideDistX, sideDistY;
        
        if (rayDirX < 0) {
            stepX = -1;
            sideDistX = (player.position.x - mapX) * deltaDistX;
        } else {
            stepX = 1;
            sideDistX = (mapX + 1.0f - player.position.x) * deltaDistX;
        }
        
        if (rayDirY < 0) {
            stepY = -1;
            sideDistY = (player.position.y - mapY) * deltaDistY;
        } else {
            stepY = 1;
            sideDistY = (mapY + 1.0f - player.position.y) * deltaDistY;
        }
        
        // DDA Algorithm
        int hit = 0;  // Was a wall hit?
        int side;     // Which side was hit? (NS or EW)
        CellType hitType = CELL_WALL; // Type of cell that was hit
        
        while (hit == 0 && (sideDistX < raycastMaxDistance || sideDistY < raycastMaxDistance)) {
            // Jump to next map square
            if (sideDistX < sideDistY) {
                sideDistX += deltaDistX;
                mapX += stepX;
                side = 0;
            } else {
                sideDistY += deltaDistY;
                mapY += stepY;
                side = 1;
            }
            
    // Check if the ray hit a wall
    if (mapX >= 0 && mapX < MAZE_WIDTH && mapY >= 0 && mapY < MAZE_HEIGHT) {
        CellType cellType = GetCellType(&gameMaze, mapX, mapY);
        if (cellType == CELL_WALL) {
            hit = 1;
            hitType = CELL_WALL;
        } 
        // else if (cellType == CELL_EXIT) {
        //     hit = 1;
        //     hitType = CELL_EXIT;
        //     }
            } else {
                hit = 1;  // Hit map boundary
            }
        }
        
        // Calculate perpendicular distance to the wall
        float perpWallDist;
        if (side == 0) {
            // Exact distance to the wall in X direction
            perpWallDist = (mapX - player.position.x + (1 - stepX) / 2) / rayDirX;
        } else {
            // Exact distance to the wall in Y direction
            perpWallDist = (mapY - player.position.y + (1 - stepY) / 2) / rayDirY;
        }
        
        // Store the distance in the z-buffer
        zBuffer[x] = perpWallDist;
        
        // Avoid division by zero
        if (perpWallDist < 0.1f) perpWallDist = 0.1f;
        
        // Calculate height of the line to draw
        int lineHeight = (int)(screenHeight / perpWallDist);
        
        // Calculate start and end points
        int drawStart = -lineHeight / 2 + screenHeight / 2;
        if (drawStart < 0) drawStart = 0;
        
        int drawEnd = lineHeight / 2 + screenHeight / 2;
        if (drawEnd >= screenHeight) drawEnd = screenHeight - 1;
        
        // Calculate texture x-coordinate
        float wallX;
        if (side == 0) {
            wallX = player.position.y + perpWallDist * rayDirY;
        } else {
            wallX = player.position.x + perpWallDist * rayDirX;
        }
        wallX -= floor(wallX);
        
        // Texture x-coordinate
        int texX = (int)(wallX * wallTextures[0].width);
        if ((side == 0 && rayDirX > 0) || (side == 1 && rayDirY < 0)) {
            texX = wallTextures[0].width - texX - 1;
        }

        Rectangle srcRect = { (float)texX, 0, 1.0f, (float)wallTextures[0].height };
        Rectangle destRect = { (float)x, (float)drawStart, 1.0f, (float)(drawEnd - drawStart) };
        Vector2 origin = { 0, 0 };
            
        // Apply shading for N/S walls
        Color tint = WHITE;
        if (side == 1) {
            tint = (Color){ 180, 180, 180, 255 }; // Gray for shading
        }
        if(hitType == CELL_EXIT){
            if (player.keys >= MAX_KEYS) {
                tint = (Color){ 50, 255, 50, 255 }; // Green for exit
            } else {
                tint = (Color){ 0, 150, 0, 255 }; // Red for blocked exit
            }
        }
            
        DrawTexturePro(wallTextures[0], srcRect, destRect, origin, 0.0f, tint);
    }
}

static void RenderSprites(float zBuffer[]) {
    // Calculate camera direction and plane
    float playerDirX = cosf(player.angle);
    float playerDirY = sinf(player.angle);
    float fovHalf = DegToRad(FOV / 2);
    float planeX = -playerDirY * tanf(fovHalf);
    float planeY = playerDirX * tanf(fovHalf);
    
    // Update the door sprite texture based on its state
    if (exitDoor.spriteIndex >= 0 && exitDoor.spriteIndex < spriteCount) {
        sprites[exitDoor.spriteIndex].texture = exitDoor.isOpen ? 
            exitDoor.openTexture : exitDoor.closedTexture;
        sprites[exitDoor.spriteIndex].frame = (Rectangle){ 
            0, 0, 
            sprites[exitDoor.spriteIndex].texture.width, 
            sprites[exitDoor.spriteIndex].texture.height 
        };
    }

    // Sort sprites by distance (from farthest to closest)
    for (int i = 0; i < spriteCount - 1; i++) {
        for (int j = 0; j < spriteCount - i - 1; j++) {
            float dist1 = Distance(player.position.x, player.position.y, 
                                  sprites[j].position.x, sprites[j].position.y);
            float dist2 = Distance(player.position.x, player.position.y, 
                                  sprites[j+1].position.x, sprites[j+1].position.y);
            
            if (dist1 < dist2) {
                // Swap sprites
                Sprite temp = sprites[j];
                sprites[j] = sprites[j+1];
                sprites[j+1] = temp;
                
                // If it's an enemy, update its sprite index
                for (int k = 0; k < enemyCount; k++) {
                    if (enemies[k].spriteIndex == j) {
                        enemies[k].spriteIndex = j+1;
                    } else if (enemies[k].spriteIndex == j+1) {
                        enemies[k].spriteIndex = j;
                    }
                }
                
                // If it's the exit door, update its index
                if (exitDoor.spriteIndex == j) {
                    exitDoor.spriteIndex = j+1;
                } else if (exitDoor.spriteIndex == j+1) {
                    exitDoor.spriteIndex = j;
                }
            }
        }
    }
    
    // Render each sprite
    for (int i = 0; i < spriteCount; i++) {
        if (!sprites[i].active) continue;
    
        // Relative position of sprite to the player
        float spriteX = sprites[i].position.x - player.position.x;
        float spriteY = sprites[i].position.y - player.position.y;
    
        // Transform to camera space
        float invDet = 1.0f / (planeX * playerDirY - playerDirX * planeY);
        float transformX = invDet * (playerDirY * spriteX - playerDirX * spriteY);
        float transformY = invDet * (-planeY * spriteX + planeX * spriteY);
    
        // If behind camera, don't render
        if (transformY <= 0) continue;
    
        // Calculate screen position
        int spriteScreenX = (int)((GetScreenWidth() / 2) * (1 + transformX / transformY));
    
        // Calculate sprite height
        int spriteHeight = abs((int)(GetScreenHeight() / transformY));
        
        // Specific adjustments by sprite type
        if (sprites[i].type == CELL_KEY) {
            spriteHeight /= 2; // Reduce height by half for keys
        }
        else if (sprites[i].type == CELL_EXIT) {
            spriteHeight = (int)(spriteHeight * 1.3f); // Increase door size by 30%
        }
    
        // Calculate sprite width - use same proportion as height to maintain shape
        int spriteWidth = spriteHeight;
        
        // Calculate drawing coordinates
        int drawStartY;
        int drawEndY;
        
        if (sprites[i].type == CELL_ENEMY) {
            // For enemies: standard position, no special adjustments
            drawStartY = -spriteHeight / 2 + GetScreenHeight() / 2;
            if (drawStartY < 0) drawStartY = 0;
            
            drawEndY = spriteHeight / 2 + GetScreenHeight() / 2 + 100;
            if (drawEndY >= GetScreenHeight()) drawEndY = GetScreenHeight() - 1;
        } 
        else if (sprites[i].type == CELL_EXIT) {
            // For doors: adjust vertical position to be on the floor
            drawEndY = GetScreenHeight() / 2 + spriteHeight / 2;
            if (drawEndY >= GetScreenHeight()) drawEndY = GetScreenHeight() - 1;
            
            drawStartY = drawEndY - spriteHeight;
            if (drawStartY < 0) drawStartY = 0;
        }
        else {
            // For keys: adjust vertical position to float at mid-height
            drawStartY = -spriteHeight / 2 + GetScreenHeight() / 2;
            if (drawStartY < 0) drawStartY = 0;
            
            drawEndY = spriteHeight / 2 + GetScreenHeight() / 2;
            if (drawEndY >= GetScreenHeight()) drawEndY = GetScreenHeight() - 1;
        }
    
        int drawStartX = -spriteWidth / 2 + spriteScreenX;
        if (drawStartX < 0) drawStartX = 0;
    
        int drawEndX = spriteWidth / 2 + spriteScreenX;
        if (drawEndX >= GetScreenWidth()) drawEndX = GetScreenWidth() - 1;
    
        // Draw the sprite
        for (int stripe = drawStartX; stripe < drawEndX; stripe++) {
            // Only draw if it's closer than a wall
            if (transformY < zBuffer[stripe]) {
                // Calculate the X coordinate in the texture
                int texX = (int)((stripe - drawStartX) * sprites[i].texture.width / (drawEndX - drawStartX));
                
                if (texX < 0) texX = 0;
                if (texX >= sprites[i].texture.width) texX = sprites[i].texture.width - 1;
                
                Rectangle srcRect = (Rectangle){ 
                    (float)texX, 
                    0, 
                    1.0f, 
                    (float)sprites[i].texture.height 
                };
                
                Rectangle destRect = {
                    (float)stripe,
                    (float)drawStartY,
                    1.0f,
                    (float)(drawEndY - drawStartY)
                };
                
                // Use specific color based on sprite type
                Color tint = WHITE; // Default color
                
                if (sprites[i].type == CELL_KEY) {
                    // Keys always with normal color
                    DrawTexturePro(sprites[i].texture, srcRect, destRect, (Vector2){0, 0}, 0, tint);
                } 
                else if (sprites[i].type == CELL_ENEMY) {
                    // Enemies always with normal color
                    DrawTexturePro(sprites[i].texture, srcRect, destRect, (Vector2){0, 0}, 0, tint);
                }
                else if (sprites[i].type == CELL_EXIT) {
                    // Door without color change, use WHITE instead of GREEN or any other color
                    DrawTexturePro(sprites[i].texture, srcRect, destRect, (Vector2){0, 0}, 0, WHITE);
                }
            }
        }
    }
}

// Render the user interface (HUD)
static void RenderHUD(void) {
    // Draw health bar
    DrawRectangle(10, GetScreenHeight() - 30, player.health * 2, 20, RED);
    DrawRectangleLines(10, GetScreenHeight() - 30, 200, 20, WHITE);
    
    // Show collected keys
    for (int i = 0; i < player.keys; i++) {
        DrawRectangle(10 + i * 20, GetScreenHeight() - 55, 15, 15, YELLOW);
    }

    if (showExitMessage) {
        const char* message = TextFormat("You need %d more keys to exit", MAX_KEYS - player.keys);
        int textWidth = MeasureText(message, 20);
        DrawRectangle(GetScreenWidth()/2 - textWidth/2 - 10, GetScreenHeight()/2 - 15, 
                     textWidth + 20, 30, (Color){0, 0, 0, 200});
        DrawText(message, GetScreenWidth()/2 - textWidth/2, GetScreenHeight()/2 - 10, 20, RED);
    }
    
    // Show weapon with animation
    if (katanaAnim.frames[0].id == 0) {
        // If textures aren't loaded, show error message
        DrawText("ERROR: Weapon textures not loaded", 10, GetScreenHeight() - 80, 20, RED);
        return;
    }

    // Calculate which frame to show based on animation state
    int frameToShow = 0;

    if (katanaAnim.isSwinging) {
        // Calculate animation progress (0.0 to 1.0)
        float progress = 1.0f - (katanaAnim.swingTimer / katanaAnim.swingDuration);
        
        // Map progress to the 5 frames (more directly)
        frameToShow = (int)(progress * 4.99f); // Using 4.99 to ensure it reaches the last frame
        
        // Ensure index is within correct range
        if (frameToShow >= 5) frameToShow = 4;
        if (frameToShow < 0) frameToShow = 0;
    } 

    // Draw the complete texture directly
    // DO NOT use DrawTexturePro which may cause scaling issues
    DrawTextureEx(
        katanaAnim.frames[frameToShow], 
        (Vector2){ GetScreenWidth() - katanaAnim.frames[frameToShow].width * katanaAnim.scale - 400,
                  GetScreenHeight() - katanaAnim.frames[frameToShow].height * katanaAnim.scale - 200 },
        0.0f,               // Rotation
        katanaAnim.scale * 1.5f,   // Scale
        WHITE               // Color
    );

    // Draw crosshair in the center of the screen
    DrawLine(crosshair.position.x - crosshair.size, crosshair.position.y, 
             crosshair.position.x + crosshair.size, crosshair.position.y, crosshair.color);
    DrawLine(crosshair.position.x, crosshair.position.y - crosshair.size, 
             crosshair.position.x, crosshair.position.y + crosshair.size, crosshair.color);
}

// ----------------------------------------------------------------------------------
// Implementation of Public Functions 
// ----------------------------------------------------------------------------------

// Initialize the game
void InitGame(void) {
    // Set initial state
    currentState = GAME_TITLE;
    
    // Initialize raycasting variables
    projPlaneDistance = (GetScreenWidth() / 2.0f) / tanf(DegToRad(FOV / 2.0f));
    numRays = GetScreenWidth();
    
    // Initialize random seed
    srand(GetTime() * 1000.0f);
    
    // Generate maze
    GenerateMaze(&gameMaze);
    
    // Initialize player
    InitPlayer();
    
    // Load textures and sounds
    wallTextures[0] = LoadTexture("assets/textures/wall.png");
    weaponTexture = LoadTexture("assets/textures/weapons/tile003.png");
    keyTexture = LoadTexture("assets/textures/key.png");    
    exitDoor.openTexture = LoadTexture("assets/textures/door_open.png");
    exitDoor.closedTexture = LoadTexture("assets/textures/door_closed.png");
    // Debug check for textures
    if (keyTexture.id == 0) {
        printf("ERROR: Failed to load key.png texture\n");
    } else {
        printf("Key texture loaded successfully. Width: %d, Height: %d\n", 
              keyTexture.width, keyTexture.height);
    }
    if (exitDoor.openTexture.id == 0) {
        printf("ERROR: Failed to load door_open.png\n");
    } else {
        printf("Door open texture loaded successfully. Width: %d, Height: %d\n", 
               exitDoor.openTexture.width, exitDoor.openTexture.height);
    }
    
    if (exitDoor.closedTexture.id == 0) {
        printf("ERROR: Failed to load door_closed.png\n");
    } else {
        printf("Door closed texture loaded successfully. Width: %d, Height: %d\n", 
               exitDoor.closedTexture.width, exitDoor.closedTexture.height);
    }
    
    exitDoor.isOpen = false;
    exitDoor.active = true;
    exitDoor.position = GridToWorld((int)gameMaze.exitPos.x, (int)gameMaze.exitPos.y);
    exitDoor.spriteIndex = -1; // Will be set when we add the sprite

    
    
    // Door position (will be updated after initializing the maze)
    //exitDoor.position = GridToWorld((int)gameMaze.exitPos.x, (int)gameMaze.exitPos.y);
    
    // Load individual enemy textures
    LoadEnemyTextures();
    
    // Initialize enemies (this also initializes sprites)
    InitEnemies();
    
    // Load sounds
    footstepSound = LoadSound("assets/sounds/footsteps.wav");
    victorySound = LoadSound("assets/sounds/victory.wav");
    keyPickupSound = LoadSound("assets/sounds/key.wav");
    playerHitSound = LoadSound("assets/sounds/damage.mp3"); 
    gameOverSound = LoadSound("assets/sounds/victory.wav"); 
    shootSound = LoadSound("assets/sounds/shoot.mp3"); 
}

// Update game logic
void UpdateGame(void) {
    switch (currentState) {
        case GAME_TITLE:
            if (IsKeyPressed(KEY_ENTER)) {
                currentState = GAME_PLAYING;
            }
            break;
            
        case GAME_PLAYING:
            // Process player movement
            MovePlayer();
            
            // Update enemies
            UpdateEnemies();
            
            // Shoot
            if (IsKeyPressed(KEY_SPACE) && !katanaAnim.isSwinging) {
                katanaAnim.isSwinging = true;
                katanaAnim.swingTimer = katanaAnim.swingDuration;
                katanaAnim.currentFrame = 0;
                katanaAnim.frameTimer = 0.0f;
                player.shootCooldown = PLAYER_SHOOT_COOLDOWN;
                PlaySound(shootSound); // Play shooting sound
            }
            
            if (katanaAnim.isSwinging) {
                katanaAnim.swingTimer -= GetFrameTime();
                katanaAnim.frameTimer += GetFrameTime();
                
                // Update animation frame based on progress
                float progress = 1.0f - (katanaAnim.swingTimer / katanaAnim.swingDuration);
                katanaAnim.currentFrame = (int)(progress * 4.99f);
                if (katanaAnim.currentFrame >= 5) katanaAnim.currentFrame = 4;
                
                // Check attacks against enemies
                CheckPlayerAttacks();
                
                if (katanaAnim.swingTimer <= 0.0f) {
                    katanaAnim.isSwinging = false;
                    katanaAnim.currentFrame = 0; // Return to first frame when finished
                }
            }

            // Update cooldown
            if (player.shootCooldown > 0) {
                player.shootCooldown -= GetFrameTime();
            }
            
            // Pause game
            if (IsKeyPressed(KEY_P)) {
                currentState = GAME_PAUSED;
            }
            break;
            
        case GAME_PAUSED:
            // Return to game
            if (IsKeyPressed(KEY_P)) {
                currentState = GAME_PLAYING;
            }
            break;
            
        case GAME_VICTORY:
        case GAME_OVER:
            // Restart game when pressing R
            if (IsKeyPressed(KEY_R)) {
                // Regenerate maze
                GenerateMaze(&gameMaze);
                
                // Reset player
                InitPlayer();
                
                // Reset enemies
                InitEnemies();
                
                // Return to game state
                currentState = GAME_PLAYING;
            }
            break;
    }
}

// Render the game
void RenderGame(void) {
    BeginDrawing();
    
    switch (currentState) {
        case GAME_TITLE:
        // Clean black background
        ClearBackground(BLACK);
        
        // Game title - larger and more prominent
        DrawText("MazeRay", 
                 GetScreenWidth()/2 - MeasureText("MazeRay", 60)/2, 
                 GetScreenHeight()/6, 
                 60, RED);
        
        // Separator line
        DrawLine(
            GetScreenWidth()/4, 
            GetScreenHeight()/6 + 80, 
            GetScreenWidth()*3/4, 
            GetScreenHeight()/6 + 80, 
            (Color){100, 100, 100, 255});
        
        // Game description - more detailed and centered
        DrawText("OBJECTIVE:", 
                 GetScreenWidth()/2 - MeasureText("OBJECTIVE:", 25)/2, 
                 GetScreenHeight()/6 + 100, 
                 25, YELLOW);
                 
        DrawText("Navigate through a dangerous maze filled with enemies", 
                 GetScreenWidth()/2 - MeasureText("Navigate through a dangerous maze filled with enemies", 18)/2, 
                 GetScreenHeight()/6 + 135, 
                 18, WHITE);
                 
        DrawText("Find all 3 keys to unlock the exit", 
                 GetScreenWidth()/2 - MeasureText("Find all 3 keys to unlock the exit", 18)/2, 
                 GetScreenHeight()/6 + 160, 
                 18, WHITE);
                 
        DrawText("Fight enemies with your 9mm to survive", 
                 GetScreenWidth()/2 - MeasureText("Fight enemies with your 9mm to survive", 18)/2, 
                 GetScreenHeight()/6 + 185, 
                 18, WHITE);
        
        // Separator line
        DrawLine(
            GetScreenWidth()/4, 
            GetScreenHeight()/2, 
            GetScreenWidth()*3/4, 
            GetScreenHeight()/2, 
            (Color){100, 100, 100, 255});
        
        // Controls - Title
        DrawText("CONTROLS:", 
                 GetScreenWidth()/2 - MeasureText("CONTROLS:", 25)/2,
                 GetScreenHeight()/2 + 30, 
                 25, YELLOW);
        
        // Controls - Movement
        DrawText("WASD / Arrow Keys - Move", 
                 GetScreenWidth()/2 - MeasureText("WASD / Arrow Keys - Move", 18)/2,
                 GetScreenHeight()/2 + 65, 
                 18, LIGHTGRAY);
        
        // Controls - Attack
        DrawText("SPACE - Shoot", 
                 GetScreenWidth()/2 - MeasureText("SPACE - Shoot", 18)/2,
                 GetScreenHeight()/2 + 95, 
                 18, LIGHTGRAY);
        
        // Controls - Pause
        DrawText("P - Pause Game", 
                 GetScreenWidth()/2 - MeasureText("P - Pause Game", 18)/2,
                 GetScreenHeight()/2 + 125, 
                 18, LIGHTGRAY);
        
        // Separator line
        DrawLine(
            GetScreenWidth()/3, 
            GetScreenHeight()*3/4, 
            GetScreenWidth()*2/3, 
            GetScreenHeight()*3/4, 
            (Color){100, 100, 100, 255});
        
        // Instruction to start - more visible with animation
        float pulse = sinf(GetTime() * 4) * 0.5f + 0.5f;
        Color startColor = ColorAlpha(GREEN, 0.5f + 0.5f * pulse);
        
        DrawText("Press ENTER to start", 
                 GetScreenWidth()/2 - MeasureText("Press ENTER to start", 24)/2,
                 GetScreenHeight()*3/4 + 30, 
                 24, startColor);
        
        break;
            
        case GAME_PLAYING:
            {
                // Create z-buffer
                float zBuffer[GetScreenWidth()];
                
                // Render 3D view
                RenderRaycasting(zBuffer);
                RenderSprites(zBuffer);
                
                // Render HUD
                RenderHUD();
            }
            break;
            
        case GAME_PAUSED:
            // Show pause screen over the game
            {
                float zBuffer[GetScreenWidth()];
                RenderRaycasting(zBuffer);
                DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){0, 0, 0, 150});
                DrawText("PAUSED", GetScreenWidth()/2 - MeasureText("PAUSED", 40)/2, GetScreenHeight()/2 - 40, 40, WHITE);
                DrawText("Press P to continue", GetScreenWidth()/2 - MeasureText("Press P to continue", 20)/2, GetScreenHeight()/2 + 10, 20, LIGHTGRAY);
            }
            break;
            
            case GAME_VICTORY:
            // Improved victory screen
            ClearBackground(BLACK);
            
            // Victory title - larger and eye-catching
            DrawText("VICTORY!", 
                     GetScreenWidth()/2 - MeasureText("VICTORY!", 60)/2, 
                     GetScreenHeight()/4, 
                     60, GREEN);
            
            // Congratulation message
            DrawText("Congratulations! You escaped from the maze", 
                     GetScreenWidth()/2 - MeasureText("Congratulations! You escaped from the maze", 20)/2, 
                     GetScreenHeight()/4 + 70, 
                     20, RAYWHITE);
            
            // Separator line
            DrawLine(
                GetScreenWidth()/4, 
                GetScreenHeight()/2, 
                GetScreenWidth()*3/4, 
                GetScreenHeight()/2, 
                (Color){100, 100, 100, 255});
            
            // Thank you message
            DrawText("Thanks for playing MazeRay!", 
                     GetScreenWidth()/2 - MeasureText("Thanks for playing MazeRay!", 24)/2, 
                     GetScreenHeight()/2 + 30, 
                     24, YELLOW);
            
            // Credits or additional information
            DrawText("A raylib game created by Jorge Carrascosa", 
                     GetScreenWidth()/2 - MeasureText("A raylib game created by Jorge Carrascosa", 18)/2, 
                     GetScreenHeight()/2 + 65, 
                     18, GRAY);
            
            // Separator line
            DrawLine(
                GetScreenWidth()/3, 
                GetScreenHeight()*3/4 - 30, 
                GetScreenWidth()*2/3, 
                GetScreenHeight()*3/4 - 30, 
                (Color){100, 100, 100, 255});
            
            // Instruction to retry with pulsing effect
            float pulseVictory = sinf(GetTime() * 4) * 0.5f + 0.5f;
            Color victoryRestartColor = ColorAlpha(WHITE, 0.5f + 0.5f * pulseVictory);
            
            DrawText("Press R to play again", 
                     GetScreenWidth()/2 - MeasureText("Press R to play again", 24)/2, 
                     GetScreenHeight()*3/4, 
                     24, victoryRestartColor);
            break;
            
            case GAME_OVER:
            // Improved defeat screen
            ClearBackground(BLACK);
            
            // Main title - larger and dramatic
            DrawText("GAME OVER", 
                     GetScreenWidth()/2 - MeasureText("GAME OVER", 60)/2, 
                     GetScreenHeight()/4, 
                     60, RED);
            
            // Message detailing the cause of death
            DrawText("YOU WERE KILLED BY ENEMIES!", 
                     GetScreenWidth()/2 - MeasureText("YOU WERE KILLED BY ENEMIES!", 24)/2, 
                     GetScreenHeight()/4 + 70, 
                     24, RAYWHITE);
            
            // Separator line
            DrawLine(
                GetScreenWidth()/4, 
                GetScreenHeight()/2, 
                GetScreenWidth()*3/4, 
                GetScreenHeight()/2, 
                (Color){100, 100, 100, 255});
            
            // Statistics or motivational message
            DrawText("The maze remains unconquered...", 
                     GetScreenWidth()/2 - MeasureText("The maze remains unconquered...", 18)/2, 
                     GetScreenHeight()/2 + 30, 
                     18, GRAY);
            
            // Separator line
            DrawLine(
                GetScreenWidth()/3, 
                GetScreenHeight()*3/4 - 30, 
                GetScreenWidth()*2/3, 
                GetScreenHeight()*3/4 - 30, 
                (Color){100, 100, 100, 255});
            
            // Instruction to retry with pulsing effect
            float pulseGameOver = sinf(GetTime() * 4) * 0.5f + 0.5f;
            Color restartColor = ColorAlpha(WHITE, 0.5f + 0.5f * pulseGameOver);
            
            DrawText("Press R to try again", 
                     GetScreenWidth()/2 - MeasureText("Press R to try again", 24)/2, 
                     GetScreenHeight()*3/4, 
                     24, restartColor);
            break;
    }
    
    EndDrawing();
}

// Free resources
void CloseGame(void) {
    // Unload textures
    UnloadTexture(wallTextures[0]);
    UnloadTexture(weaponTexture);
    UnloadTexture(keyTexture);
    
    // Unload weapon frame textures
    for (int i = 0; i < 5; i++) {
        UnloadTexture(katanaAnim.frames[i]);
    }
    
    // Unload individual enemy textures
    for (int i = 0; i < 56; i++) {
        UnloadTexture(impTextures[i]);
    }
    
    // Unload door textures
    UnloadTexture(exitDoor.openTexture);
    UnloadTexture(exitDoor.closedTexture);
    
    // Unload sounds
    UnloadSound(footstepSound);
    UnloadSound(victorySound);
    UnloadSound(keyPickupSound);
    UnloadSound(playerHitSound);
    UnloadSound(gameOverSound);
    UnloadSound(shootSound); // Unload the new shooting sound
}
