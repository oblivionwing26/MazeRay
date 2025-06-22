#ifndef UTILS_H
#define UTILS_H

#include "raylib.h"
#include <math.h>

// Constants
#define PI              3.14159265358979323846f
#define DEG2RAD         (PI/180.0f)
#define RAD2DEG         (180.0f/PI)

// Distance between two points
float Distance(float x1, float y1, float x2, float y2);

// Normalizar un ángulo entre 0 y 2*PI
float NormalizeAngle(float angle);

float DegToRad(float degrees);

float RadToDeg(float radians);

// Verify collision between a circle and a rectangle
bool CheckCollisionCircleRec(Vector2 center, float radius, Rectangle rec);

// Limit a value between min and max
float Clamp(float value, float min, float max);

// Interpolate linearly between two values
float Lerp(float start, float end, float amount);

Color GetTexturePixelColor(Texture2D texture, int x, int y);

#endif // UTILS_H