#include "spaceship.h"
#include "solarSystem.h"
#include "audio.h"

// =============================================================================
//  Global State
// =============================================================================
Spaceship g_spaceship;
bool g_gameMode = false;
float g_cameraDistance = 5.0f;

// =============================================================================
//  Initialization
// =============================================================================
static void drawSpaceshipModel();

void initSpaceship() {
    g_spaceship = Spaceship();  // Reset to default state
}

// =============================================================================
//  Direction Vector Helpers
// =============================================================================

glm::vec3 getSpaceshipForward() {
    // Calculate forward direction from yaw and pitch
    glm::vec3 forward;
    forward.x = sinf(g_spaceship.yaw) * cosf(g_spaceship.pitch);
    forward.y = -sinf(g_spaceship.pitch);
    forward.z = -cosf(g_spaceship.yaw) * cosf(g_spaceship.pitch);
    return glm::normalize(forward);
}

glm::vec3 getSpaceshipRight() {
    // Cross product of forward and world up to get right vector
    glm::vec3 forward = getSpaceshipForward();
    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
    // Fallback if forward is parallel to up
    if (glm::length(right) < 0.001f) {
        right = glm::vec3(1.0f, 0.0f, 0.0f);
    }
    return right;
}

glm::vec3 getSpaceshipUp() {
    // Cross product of right and forward to get up vector
    glm::vec3 forward = getSpaceshipForward();
    glm::vec3 right = getSpaceshipRight();
    return glm::normalize(glm::cross(right, forward));
}

// =============================================================================
//  Spaceship Physics Update
// =============================================================================

void updateSpaceship(float deltaTime) {
    GLFWwindow* win = glfwGetCurrentContext();
    if (!win) return;

    // Get direction vectors
    glm::vec3 forward = getSpaceshipForward();
    glm::vec3 horizontalForward = glm::normalize(glm::vec3(forward.x, 0.0f, forward.z));

    // --- Speed Control (W/S) ---
    float speedChange = 0.0f;
    bool moving = false;

    if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS) {
        speedChange = g_spaceship.acceleration * deltaTime;
        moving = true;
    }
    if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS) {
        speedChange = -g_spaceship.acceleration * deltaTime;
        moving = true;
    }

    // Engine sound
    g_audio.playEngine(moving);

    // Apply acceleration or friction
    if (moving) {
        g_spaceship.speed += speedChange;
    }
    else {
        g_spaceship.speed *= g_spaceship.friction;
        if (fabs(g_spaceship.speed) < 0.001f) g_spaceship.speed = 0.0f;
    }

    // --- Boost (Left Shift) ---
    bool wasBoost = g_spaceship.boostActive;
    g_spaceship.boostActive = (glfwGetKey(win, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS);
    if (g_spaceship.boostActive != wasBoost) {
        g_audio.playBoost(g_spaceship.boostActive);
    }

    float currentMaxSpeed = g_spaceship.maxSpeed;
    if (g_spaceship.boostActive) {
        currentMaxSpeed *= g_spaceship.boostMultiplier;
    }
    g_spaceship.speed = glm::clamp(g_spaceship.speed, -currentMaxSpeed * 0.3f, currentMaxSpeed);

    // --- Turning (A/D) ---
    const float TURN_SPEED = 2.5f * deltaTime;
    if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS) {
        g_spaceship.yaw += TURN_SPEED;
        g_spaceship.rollAngle = -0.4f;  // Visual roll left
    }
    else if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS) {
        g_spaceship.yaw -= TURN_SPEED;
        g_spaceship.rollAngle = 0.4f;   // Visual roll right
    }
    else {
        // Smoothly return roll to neutral
        g_spaceship.rollAngle *= 0.9f;
        if (fabs(g_spaceship.rollAngle) < 0.001f) g_spaceship.rollAngle = 0.0f;
    }

    // --- Vertical Movement (Space for up, Left Control for down) ---
    const float VERTICAL_SPEED = g_spaceship.acceleration * deltaTime * 1.5f;  // Faster vertical movement
    float verticalDelta = 0.0f;

    // Space bar for upward movement
    if (glfwGetKey(win, GLFW_KEY_SPACE) == GLFW_PRESS) {
        verticalDelta = VERTICAL_SPEED;
    }
    // Left Control for downward movement
    if (glfwGetKey(win, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
        verticalDelta = -VERTICAL_SPEED;
    }

    // --- Apply Movement ---
    // Move forward/backward HORIZONTALLY ONLY (no vertical component)
    g_spaceship.position += horizontalForward * g_spaceship.speed * deltaTime;
    // Vertical movement is independent (Space/LControl)
    g_spaceship.position.y += verticalDelta;

    // Update velocity for physics tracking
    g_spaceship.velocity = horizontalForward * g_spaceship.speed + glm::vec3(0.0f, verticalDelta, 0.0f);

    // --- Collision Detection with Planets ---
    for (int i = 0; i < NUM_PLANETS; i++) {
        glm::vec3 planetPos(g_planet[i].x, g_planet[i].y, g_planet[i].z);
        glm::vec3 diff = g_spaceship.position - planetPos;
        float dist = glm::length(diff);
        float threshold = g_planet[i].radius * 2.0f;

        if (dist < threshold && dist > 0.001f) {
            glm::vec3 normal = glm::normalize(diff);
            g_spaceship.position = planetPos + normal * threshold;
            float velAlongNormal = glm::dot(g_spaceship.velocity, normal);
            if (velAlongNormal < 0) {
                g_spaceship.velocity -= normal * velAlongNormal * 1.2f;
                g_spaceship.speed = glm::length(g_spaceship.velocity) *
                    (glm::dot(g_spaceship.velocity, horizontalForward) > 0 ? 1.0f : -1.0f);
            }
        }
    }

    // Clamp pitch to prevent over-rotation
    g_spaceship.pitch = glm::clamp(g_spaceship.pitch, -0.0f, 0.2630f);
}

// =============================================================================
//  Game Camera Update
// =============================================================================

void updateGameCamera(float cameraDistance, float heightOffset) {
    // Camera looks at the ship's position
    glm::vec3 lookTarget = g_spaceship.position;

    // Calculate camera position behind and above the ship
    glm::vec3 forward = getSpaceshipForward();
    glm::vec3 up = getSpaceshipUp();

    // Scale camera distance to match ship's small scale
    const float CAMERA_SCALE = 0.0025f;
    glm::vec3 camOffset = -forward * cameraDistance * CAMERA_SCALE
        + up * heightOffset * CAMERA_SCALE;
    glm::vec3 camPos = g_spaceship.position + camOffset;

    // Update global camera
    g_camera.position = camPos;
    g_camera.target = lookTarget;
    g_camera.mode = Camera::Mode::FREE;
}

// =============================================================================
//  Rendering
// =============================================================================

void renderSpaceship() {
    glPushMatrix();

    // Position the ship in world space
    glTranslatef(g_spaceship.position.x, g_spaceship.position.y, g_spaceship.position.z);

    // Apply orientation rotations
    // Order: roll -> pitch -> yaw -> final 180 flip to correct front/back
    glRotatef(-glm::degrees(g_spaceship.yaw), 0.0f, 1.0f, 0.0f);
    glRotatef(glm::degrees(g_spaceship.pitch), 1.0f, 0.0f, 0.0f);
    glRotatef(glm::degrees(g_spaceship.rollAngle), 0.0f, 0.0f, 1.0f);
    glRotatef(180.0f, 0.0f, 1.0f, 0.0f);  

    drawSpaceshipModel();

    glPopMatrix();
}
// =============================================================================
//  Collision Detection
// =============================================================================

static void checkPlanetCollisions() {
    for (int i = 0; i < NUM_PLANETS; i++) {
        glm::vec3 planetPos(g_planet[i].x, g_planet[i].y, g_planet[i].z);
        glm::vec3 diff = g_spaceship.position - planetPos;
        float dist = glm::length(diff);
        float threshold = g_planet[i].radius * 2.0f; // Ship is small, use planet radius as collision boundary

        if (dist < threshold && dist > 0.001f) {
            // Push ship out along the normal direction
            glm::vec3 normal = glm::normalize(diff);
            g_spaceship.position = planetPos + normal * threshold;
            // Stop movement into the planet
            float velAlongNormal = glm::dot(g_spaceship.velocity, normal);
            if (velAlongNormal < 0) {
                g_spaceship.velocity -= normal * velAlongNormal * 1.2f;
                g_spaceship.speed = glm::length(g_spaceship.velocity) *
                    (glm::dot(g_spaceship.velocity, getSpaceshipForward()) > 0 ? 1.0f : -1.0f);
            }
            // Play collision effect (optional)
            // g_audio.playCollision();
        }
    }
}


// =============================================================================
//  Spaceship Model Drawing
// =============================================================================

/**
 * Draws a detailed spaceship model using immediate mode OpenGL
 * The model is drawn at unit scale; camera distance handles proper sizing
 */
void drawSpaceshipModel() {
    // Disable lighting for flat-shaded look
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    const float SHIP_SCALE = 0.0025f;
    glPushMatrix();
    glScalef(SHIP_SCALE, SHIP_SCALE, SHIP_SCALE);

    // --- Main Fuselage (body) ---
    glColor3f(0.75f, 0.75f, 0.85f);
    glBegin(GL_QUADS);
    // Top
    glVertex3f(-0.5f, 0.4f, 1.0f);
    glVertex3f(0.5f, 0.4f, 1.0f);
    glVertex3f(0.5f, 0.4f, -1.0f);
    glVertex3f(-0.5f, 0.4f, -1.0f);
    // Bottom
    glVertex3f(-0.5f, -0.4f, 1.0f);
    glVertex3f(-0.5f, -0.4f, -1.0f);
    glVertex3f(0.5f, -0.4f, -1.0f);
    glVertex3f(0.5f, -0.4f, 1.0f);
    // Left side
    glVertex3f(-0.5f, -0.4f, 1.0f);
    glVertex3f(-0.5f, -0.4f, -1.0f);
    glVertex3f(-0.5f, 0.4f, -1.0f);
    glVertex3f(-0.5f, 0.4f, 1.0f);
    // Right side
    glVertex3f(0.5f, -0.4f, 1.0f);
    glVertex3f(0.5f, 0.4f, 1.0f);
    glVertex3f(0.5f, 0.4f, -1.0f);
    glVertex3f(0.5f, -0.4f, -1.0f);
    glEnd();

    // --- Nose Cone ---
    glColor3f(0.6f, 0.6f, 0.75f);
    glBegin(GL_TRIANGLES);
    // Four triangular faces forming a pointed nose
    // Bottom
    glVertex3f(0.0f, 0.0f, 1.8f);
    glVertex3f(-0.4f, -0.3f, 1.0f);
    glVertex3f(0.4f, -0.3f, 1.0f);
    // Right
    glVertex3f(0.0f, 0.0f, 1.8f);
    glVertex3f(0.4f, -0.3f, 1.0f);
    glVertex3f(0.4f, 0.3f, 1.0f);
    // Top
    glVertex3f(0.0f, 0.0f, 1.8f);
    glVertex3f(0.4f, 0.3f, 1.0f);
    glVertex3f(-0.4f, 0.3f, 1.0f);
    // Left
    glVertex3f(0.0f, 0.0f, 1.8f);
    glVertex3f(-0.4f, 0.3f, 1.0f);
    glVertex3f(-0.4f, -0.3f, 1.0f);
    glEnd();

    // --- Wings ---
    glColor3f(0.65f, 0.65f, 0.8f);
    glBegin(GL_QUADS);
    // Left wing
    glVertex3f(-0.4f, -0.1f, 0.3f);
    glVertex3f(-1.2f, -0.1f, 0.5f);
    glVertex3f(-1.2f, -0.1f, -0.2f);
    glVertex3f(-0.4f, -0.1f, 0.0f);
    // Right wing
    glVertex3f(0.4f, -0.1f, 0.3f);
    glVertex3f(0.4f, -0.1f, 0.0f);
    glVertex3f(1.2f, -0.1f, -0.2f);
    glVertex3f(1.2f, -0.1f, 0.5f);
    glEnd();

    // --- Wing Tip Lights (red navigation) ---
    glColor3f(0.9f, 0.1f, 0.1f);
    glBegin(GL_QUADS);
    // Left wing tip
    glVertex3f(-1.2f, -0.1f, 0.5f);
    glVertex3f(-1.2f, -0.1f, -0.2f);
    glVertex3f(-1.2f, 0.1f, -0.2f);
    glVertex3f(-1.2f, 0.1f, 0.5f);
    // Right wing tip
    glVertex3f(1.2f, -0.1f, 0.5f);
    glVertex3f(1.2f, 0.1f, 0.5f);
    glVertex3f(1.2f, 0.1f, -0.2f);
    glVertex3f(1.2f, -0.1f, -0.2f);
    glEnd();

    // --- Tail Fins ---
    glColor3f(0.55f, 0.55f, 0.7f);
    glBegin(GL_QUADS);
    // Vertical stabilizer
    glVertex3f(-0.05f, 0.4f, -0.5f);
    glVertex3f(0.05f, 0.4f, -0.5f);
    glVertex3f(0.05f, 0.8f, -0.8f);
    glVertex3f(-0.05f, 0.8f, -0.8f);
    // Left stabilizer
    glVertex3f(-0.4f, -0.1f, -0.5f);
    glVertex3f(-0.7f, -0.1f, -0.8f);
    glVertex3f(-0.7f, 0.1f, -0.8f);
    glVertex3f(-0.4f, 0.1f, -0.5f);
    // Right stabilizer
    glVertex3f(0.4f, -0.1f, -0.5f);
    glVertex3f(0.4f, 0.1f, -0.5f);
    glVertex3f(0.7f, 0.1f, -0.8f);
    glVertex3f(0.7f, -0.1f, -0.8f);
    glEnd();

    // --- Cockpit (glass, translucent blue) ---
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.0f, 0.6f, 1.0f, 0.6f);
    glBegin(GL_QUADS);
    // Front windshield
    glVertex3f(-0.25f, 0.2f, 0.4f);
    glVertex3f(0.25f, 0.2f, 0.4f);
    glVertex3f(0.25f, 0.4f, 0.1f);
    glVertex3f(-0.25f, 0.4f, 0.1f);
    // Left window
    glVertex3f(-0.4f, 0.2f, 0.4f);
    glVertex3f(-0.4f, 0.35f, 0.1f);
    glVertex3f(-0.4f, 0.35f, -0.1f);
    glVertex3f(-0.4f, 0.2f, -0.1f);
    // Right window
    glVertex3f(0.4f, 0.2f, 0.4f);
    glVertex3f(0.4f, 0.2f, -0.1f);
    glVertex3f(0.4f, 0.35f, -0.1f);
    glVertex3f(0.4f, 0.35f, 0.1f);
    glEnd();
    glDisable(GL_BLEND);

    // --- Engine Flame (visible when moving) ---
    if (fabs(g_spaceship.speed) > 0.1f) {
        float speedPercent = fabs(g_spaceship.speed) / g_spaceship.maxSpeed;
        float intensity = glm::clamp(speedPercent, 0.0f, 1.0f);
        float flameLength = 0.4f + intensity * 1.0f;

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Outer flame (orange)
        glColor4f(1.0f, 0.5f, 0.0f, intensity * 0.7f);
        glBegin(GL_TRIANGLES);
        glVertex3f(-0.2f, -0.2f, -1.0f);
        glVertex3f(0.2f, -0.2f, -1.0f);
        glVertex3f(0.0f, -0.2f, -1.0f - flameLength);
        glVertex3f(-0.2f, 0.2f, -1.0f);
        glVertex3f(0.0f, 0.2f, -1.0f - flameLength);
        glVertex3f(0.2f, 0.2f, -1.0f);
        glEnd();

        // Inner flame (yellow-white)
        glColor4f(1.0f, 0.9f, 0.3f, intensity * 0.9f);
        float innerLength = flameLength * 0.6f;
        glBegin(GL_TRIANGLES);
        glVertex3f(-0.1f, -0.1f, -1.0f);
        glVertex3f(0.1f, -0.1f, -1.0f);
        glVertex3f(0.0f, -0.1f, -1.0f - innerLength);
        glVertex3f(-0.1f, 0.1f, -1.0f);
        glVertex3f(0.0f, 0.1f, -1.0f - innerLength);
        glVertex3f(0.1f, 0.1f, -1.0f);
        glEnd();

        // Core flame (blue-white at high speed)
        if (intensity > 0.5f) {
            glColor4f(0.6f, 0.8f, 1.0f, intensity * 0.5f);
            float coreLength = innerLength * 0.5f;
            glBegin(GL_TRIANGLES);
            glVertex3f(-0.05f, -0.05f, -1.0f);
            glVertex3f(0.05f, -0.05f, -1.0f);
            glVertex3f(0.0f, -0.05f, -1.0f - coreLength);
            glVertex3f(-0.05f, 0.05f, -1.0f);
            glVertex3f(0.0f, 0.05f, -1.0f - coreLength);
            glVertex3f(0.05f, 0.05f, -1.0f);
            glEnd();
        }
        glDisable(GL_BLEND);

        // Glow effect at engine nozzle
        glColor4f(0.5f, 0.3f, 0.0f, intensity * 0.15f);
        glPointSize(10.0f * intensity);
        glBegin(GL_POINTS);
        glVertex3f(0.0f, 0.0f, -1.0f);
        glEnd();
    }

    // --- Engine Nozzle Housing ---
    glColor3f(0.3f, 0.3f, 0.4f);
    glBegin(GL_QUADS);
    glVertex3f(-0.3f, -0.3f, -1.0f);
    glVertex3f(0.3f, -0.3f, -1.0f);
    glVertex3f(0.3f, 0.3f, -1.0f);
    glVertex3f(-0.3f, 0.3f, -1.0f);
    glEnd();

    // Engine nozzle detail ring
    glColor3f(0.2f, 0.2f, 0.3f);
    glBegin(GL_QUADS);
    glVertex3f(-0.25f, -0.25f, -1.05f);
    glVertex3f(0.25f, -0.25f, -1.05f);
    glVertex3f(0.25f, 0.25f, -1.05f);
    glVertex3f(-0.25f, 0.25f, -1.05f);
    glEnd();

    glPopMatrix();

    // Re-enable lighting for other objects
    glEnable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
}