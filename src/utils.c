#include "utils.h"
#include <math.h>

// Calculate distance between two points
float Distance(float x1, float y1, float x2, float y2) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    return sqrtf(dx*dx + dy*dy);
}

// Normalizar un ángulo entre 0 y 2*PI
float NormalizeAngle(float angle) {
    while (angle < 0) angle += 2.0f * PI;
    while (angle >= 2.0f * PI) angle -= 2.0f * PI;
    return angle;
}

// Convertir degrees to radians
float DegToRad(float degrees) {
    return degrees * DEG2RAD;
}

// Convert radians to degrees
float RadToDeg(float radians) {
    return radians * RAD2DEG;
}
Color GetTexturePixelColor(Texture2D texture, int x, int y) {
    Color* pixels = LoadImageColors(LoadImageFromTexture(texture));
    Color color = pixels[y * texture.width + x];
    UnloadImageColors(pixels);
    return color;
}

