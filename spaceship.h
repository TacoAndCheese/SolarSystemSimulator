#pragma once
#include "solarSystem.h"

// =============================================================================
//  Spaceship System - Handles player-controlled ship in game mode
// =============================================================================

/**
 * Spaceship state structure
 * Contains all parameters for ship physics, movement, and visual state
 */
struct Spaceship {
    // Position and movement
    glm::vec3 position = glm::vec3(0.0f, -30.0f, -450.0f);
    glm::vec3 velocity = glm::vec3(0.0f, 0.0f, 0.0f);
    float speed = 0.0f;

    // Orientation (in radians)
    float yaw = 0.0f;          // Horizontal rotation (left/right)
    float pitch = 0.0f;        // Vertical rotation (up/down)
    float rollAngle = 0.0f;    // Visual roll effect during turns

    // Physics parameters
    float maxSpeed = 1.0f;
    float acceleration = 0.2f;
    float friction = 0.45f;
    float boostMultiplier = 1.6f;
    bool boostActive = false;
};

// =============================================================================
//  Function Declarations
// =============================================================================

// Initialization
void initSpaceship();

// Core update functions
void updateSpaceship(float deltaTime);
void updateGameCamera(float cameraDistance, float heightOffset);

// Rendering
void renderSpaceship();
void checkPlanetCollisions();

// Direction vector helpers
glm::vec3 getSpaceshipForward();
glm::vec3 getSpaceshipRight();
glm::vec3 getSpaceshipUp();

// =============================================================================
//  External Global Variables
// =============================================================================
extern Spaceship g_spaceship;
extern bool g_gameMode;
extern float g_cameraDistance;