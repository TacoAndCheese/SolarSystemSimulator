#pragma once

// =============================================================================
//  Third-Party Includes
// =============================================================================
#define GLM_ENABLE_EXPERIMENTAL
#include "stb_image.h"

// OpenGL - GLEW must come before any other GL headers
#include <glew.h>

// Window Management
#include <glfw3.h>

// Mathematics
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

// ImGui
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

// Standard Library
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>
#include <random>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>

// =============================================================================
//  Constants
// =============================================================================

// Mathematical constants
static const float PI = static_cast<float>(acos(-1.0));

// Simulation timing
static const float FPS = 90.0f;

// Scale constants for realistic planetary representation
static const float BASE_R = 3.0f;           // Earth's orbital radius scale
static const float BASE = 0.1f;             // Earth's radius scale
static const float BASE_GZ = 2.0f * PI / (FPS * 20.0f);  // Base angular velocity
static const float BASE_ZZ = 360.0f / FPS / 5.0f;        // Base rotation speed

// Object counts
static const int NUM_PLANETS = 11;          // Sun + 8 planets + Moon + Planet X
static const int AST_COUNT = 2250;          // Asteroid belt density
static const int STAR_COUNT = 5000;         // Background star field
static const int NUM_TEX = 17;              // Total textures (planets + skybox)

// Inline utility
inline float toRad(float deg) { return deg * PI / 180.0f; }

// =============================================================================
//  Camera System
// =============================================================================

/**
 * Camera supporting both orbit and free-flight modes
 */
struct Camera {
    // Position and orientation
    glm::vec3 position = glm::vec3(0.0f, 0.0f, 10.0f);
    glm::vec3 target = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

    // Orbit mode parameters (rotating around a target)
    float orbitDistance = 10.0f;
    float orbitPitch = 0.0f;    // Vertical angle (degrees)
    float orbitYaw = 0.0f;      // Horizontal angle (degrees)

    // Free mode parameters (first-person style)
    float yaw = 0.0f;           // Horizontal rotation (degrees)
    float pitch = 0.0f;         // Vertical rotation (degrees)

    // Movement speeds
    float moveSpeed = 5.0f;
    float panSpeed = 5.0f;
    float orbitSpeed = 0.5f;
    float zoomSpeed = 2.0f;

    // Mouse interaction state
    bool isDragging = false;
    bool isPanning = false;
    double lastMouseX = 0.0;
    double lastMouseY = 0.0;

    // Camera mode
    enum class Mode { ORBIT, FREE } mode = Mode::ORBIT;

    // Direction vector helpers
    glm::vec3 forward() const;
    glm::vec3 right() const;

    // Matrix generation
    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(float aspect, float fov = 60.0f,
        float nearPlane = 0.01f, float farPlane = 2000.0f) const;
};

// =============================================================================
//  Mesh System
// =============================================================================

/**
 * Vertex structure for OpenGL immediate-mode rendering
 */
struct Vertex {
    float px, py, pz;    // Position
    float nx, ny, nz;    // Normal
    float u, v;          // Texture coordinates
};

/**
 * Mesh container with OpenGL buffer handles
 */
struct Mesh {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    int indexCount = 0;
};

// =============================================================================
//  Celestial Bodies
// =============================================================================

/**
 * Represents a celestial body (planet, moon, or asteroid)
 */
struct Body {
    // Current position
    float x = 0, y = 0, z = 0;

    // Orbit center offset (for moons orbiting planets)
    float cx = 0, cy = 0, cz = 0;

    // Orbit center translation
    float dx = 0, dy = 0;

    // Orbital properties
    float orbitRadius = 0;
    float radius = 0;
    float angle = 0;
    float angularVelocity = 0;

    // Color (used when texture is unavailable)
    float r = 1, g = 1, b = 1;

    // Texture index and self-rotation
    int number = 0;          // Index into g_tex array (-1 = no texture)
    float rotation = 0;
    float dr = 0;            // Rotation speed

    // Visual parameters
    float bias = 0;          // Visual offset for orbit rendering
    float tilt = 0;          // Axial tilt in degrees
};

/**
 * Particle for visual effects (e.g., comet tail)
 */
struct Particle {
    float x, y, z;
    float vx, vy, vz;
    float life;
};

/**
 * Background star point
 */
struct StarPoint {
    float x, y, z;
};

// =============================================================================
//  Planet Information (for UI display)
// =============================================================================

/**
 * Educational information about each celestial body
 */
struct PlanetInfo {
    char name[32];
    float distanceFromSun;   // Million km
    float orbitalSpeed;      // km/s
    float rotationPeriod;    // Earth days (negative = retrograde)
    float gravity;           // m/s² surface gravity
    char fact1[256];         // Interesting fact #1
    char fact2[256];         // Interesting fact #2
};

// =============================================================================
//  Global Variable Declarations
// =============================================================================

// Camera
extern Camera g_camera;

// Celestial bodies
extern Body g_planet[NUM_PLANETS];
extern Body g_save[NUM_PLANETS];           // Backup for switching views
extern Body g_asteroids[AST_COUNT];
extern StarPoint g_stars[STAR_COUNT];

// Particle system
extern std::vector<Particle> g_particles;

// Textures
extern GLuint g_tex[NUM_TEX];

// Meshes
extern Mesh g_sphereMesh;
extern Mesh g_satRingMesh;
extern Mesh g_urRingMesh;

// State flags
extern int g_followIdx;                    // Currently followed planet (-1 = none)
extern bool g_overview;                    // True = overview scale, false = realistic
extern int g_viewOrbit;                    // 1 = show orbit lines, 0 = hide
extern int g_selectedPlanet;               // Planet selected for info panel (-1 = none)

// Window and performance
extern int g_winW;
extern int g_winH;
extern float g_currentFPS;

// Planet information data
extern PlanetInfo g_planetInfo[NUM_PLANETS];

// Game mode globals (defined in spaceship.cpp)
extern bool g_gameMode;
extern float g_cameraDistance;

extern float g_lightIntensity;

// =============================================================================
//  Function Declarations
// =============================================================================

// ---- Initialization ----
void initPlanets();
void initStars();
void initAsteroids();
void initGL();
void loadAllTextures();

// ---- Mesh Construction ----
Mesh buildSphereMesh(int stacks, int slices);
Mesh buildDiskMesh(float innerR, float outerR, int slices);
void drawMesh(const Mesh& m);

// ---- Rendering ----
void drawSphere(float radius, GLuint texID, float r, float g, float b,
    float tilt, float rotation);
void drawBody(const Body& body);
void drawRing(const Mesh& ringMesh, float planetRadius, GLuint texID,
    float tilt, float rotAngle);
void drawOrbitCircle(float cx, float cy, float R, int number);
void drawSkybox();
void drawStars();
void drawAsteroids();
void drawParticles();

// ---- Lighting and Projection ----
void setLights();
void applyPerspective(float fovDeg, float aspect, float near_, float far_);

// ---- Planet Information ----
void setupPlanetInfo();
bool raycastPlanet(float mouseX, float mouseY, int& hitIndex);

// ---- Particle System ----
void emitParticle();
void updateParticles();

// ---- Simulation ----
void updatePlanets();
void applyOverviewScale();

// ---- Camera Controls ----
void updateOrbitCamera();
void updateFreeCamera();
void resetCamera();
void followPlanet(int idx);

// ---- Main Loop ----
void renderScene();
void renderImGui();

// ---- GLFW Callbacks ----
void cbScroll(GLFWwindow* win, double xoffset, double yoffset);
void cbMouseBtn(GLFWwindow* win, int btn, int action, int mods);
void cbCursorPos(GLFWwindow* win, double mx, double my);
void cbKey(GLFWwindow* win, int key, int scancode, int action, int mods);
void cbResize(GLFWwindow* win, int w, int h);