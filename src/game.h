#ifndef GAME_H
#define GAME_H

#include "raylib.h"
#include "maze.h"

// Game definitions
#define FOV             60.0f    // Field of view in degrees
#define PLAYER_SPEED    2.0f     // Player movement speed
#define PLAYER_ROT_SPEED 2.0f    // Player rotation speed
#define MAX_KEYS        3        // Maximum number of keys that can be collected
#define MAX_ENEMIES     10       // Maximum number of enemies in the level
#define MAX_SPRITES      15      // Maximum number of sprites in the level
#define PLAYER_SHOOT_COOLDOWN 0.5f // Cooldown time between shots
#define WALL_SCALE_FACTOR 1.2f    // Adjust to change vertical scaling of walls (increased from 1.0f)
#define TEXTURE_SCALING_QUALITY 1  // 0 = faster, 1 = better quality

// New definitions for enemies
#define ENEMY_SPEED     0.5f     // Enemy movement speed
#define ENEMY_ATTACK_RANGE 1.0f  // Distance at which the enemy can attack
#define ENEMY_ATTACK_DAMAGE 10   // Damage inflicted by the enemy
#define ENEMY_ATTACK_COOLDOWN 1.0f // Time between enemy attacks
#define ENEMY_HEALTH    30       // Initial enemy health
#define PLAYER_ATTACK_RANGE 1.5f // Player attack range
#define PLAYER_ATTACK_DAMAGE 15  // Damage inflicted by the player

// Definitions for enemy animations
#define ENEMY_ANIM_IDLE     0    // Enemy idle animation
#define ENEMY_ANIM_WALK     1    // Enemy walking animation
#define ENEMY_ANIM_ATTACK   2    // Enemy attack animation
#define ENEMY_ANIM_PAIN     3    // Enemy receiving damage animation
#define ENEMY_ANIM_DEATH    4    // Enemy death animation
#define ENEMY_FRAME_TIME    0.15f // Time between animation frames

// Definitions for weapon animation
#define WEAPON_FRAMES       5    // Number of frames in weapon animation
#define WEAPON_FRAME_TIME   0.05f // Time between frames in weapon animation

// Add the definition of shooting sound after other sound definitions
static Sound shootSound;          // Shooting sound

// Add a structure for the crosshair
typedef struct {
    Vector2 position;    // Screen position
    float size;          // Crosshair size
    Color color;         // Crosshair color
} Crosshair;

// Add the declaration of the crosshair as a global variable
static Crosshair crosshair;

// Structure for the exit door
typedef struct {
    Vector2 position;    // World position
    Texture2D openTexture;   // Texture for the open door
    Texture2D closedTexture; // Texture for the closed door
    bool isOpen;         // If the door is open or closed
    bool active;         // If the door is active
    int spriteIndex;     // Index of the associated sprite
} ExitDoor;

// Add the declaration of the door as a global variable
static ExitDoor exitDoor;

// Game states
typedef enum {
    GAME_TITLE,         // Title screen
    GAME_PLAYING,       // Actively playing
    GAME_PAUSED,        // Game paused
    GAME_VICTORY,       // Level completed
    GAME_OVER           // Player lost
} GameState;

// Player structure
typedef struct {
    Vector2 position;    // Position (x, y) on the map
    float angle;         // Direction angle in radians
    int health;          // Player health (0-100)
    int keys;            // Number of keys collected
    float shootCooldown; // Cooldown time between shots
} Player;

// Modify the WeaponAnimation structure to include animation frames
typedef struct{
    bool isSwinging;     // If the player is attacking
    float swingTimer;    // Timer for the animation
    float swingDuration; // Total animation duration
    Vector2 basePosition;// Base position of the weapon
    Vector2 swingOffset; // Current weapon position offset
    float scale;         // Weapon scale
    int currentFrame;    // Current animation frame
    float frameTimer;    // Timer for frame change
    Texture2D frames[5]; // Array to store animation frames
} WeaponAnimation;

// Animation structure
typedef struct {
    int currentAnim;     // Current animation (0=idle, 1=walk, 2=attack, 3=pain, 4=death)
    int currentFrame;    // Current animation frame
    float frameTimer;    // Timer for frame change
    bool isPlaying;      // If the animation is playing
    bool loop;           // If the animation should loop
} Animation;

typedef struct {
    Vector2 position;   // Position (x, y) on the map
    Texture2D texture;  // Base sprite texture
    bool active;        // If the sprite is active or not
    CellType type;      // Cell type (key, enemy, etc.)
    Animation anim;     // Sprite animation
    Rectangle frame;    // Current animation frame (for cropping from texture)
} Sprite;

// Improved enemy structure
typedef struct {
    Vector2 position;    // Position (x, y) on the map
    float angle;         // Direction angle
    bool active;         // If the enemy is active or not
    int health;          // Enemy health
    float attackCooldown; // Cooldown time between attacks
    float moveTimer;     // Timer for direction change
    Vector2 direction;   // Current movement direction
    Animation anim;      // Enemy animation
    int spriteIndex;     // Index of the associated sprite
    bool isDying;        // If the enemy is in death animation
} Enemy;

// Handle game initialization
void InitGame(void);

// Update game logic
void UpdateGame(void);

// Render the current frame
void RenderGame(void);

// Free resources on close
void CloseGame(void);

#endif