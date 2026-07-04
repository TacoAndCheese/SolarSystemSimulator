#define STB_IMAGE_IMPLEMENTATION
#include "solarSystem.h"
#include "spaceship.h"
#include "audio.h"

// =============================================================================
//  Performance Tracking
// =============================================================================
static double g_fpsUpdateTime = 0.0;
static int g_frameCount = 0;

// =============================================================================
//  Forward Declarations
// =============================================================================
static void renderScene();
static void renderImGui();
static void renderPlanetInfoPanel();
static void renderGameModeControls();
static void renderGameModePanel();
static void renderNormalModePanel();
static void renderFollowCombo();
static void toggleGameMode();
static void toggleViewMode();
static void adjustSimulationSpeed(float multiplier);
static void cleanupResources();

// =============================================================================
//  Scene Rendering
// =============================================================================

/**
 * Renders one frame of the solar system simulation
 */
static void renderScene() {
    // Clear the screen
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Setup projection
    float aspect = g_winH > 0 ? (float)g_winW / (float)g_winH : 1.0f;
    glViewport(0, 0, g_winW, g_winH);
    applyPerspective(60.0f, aspect, 0.01f, 2000.0f);

    // Setup modelview matrix
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Update game camera if in game mode
    if (g_gameMode) {
        updateGameCamera(g_cameraDistance, 0.0f);
    }

    // Apply camera view
    glm::mat4 view = g_camera.getViewMatrix();
    glMultMatrixf(glm::value_ptr(view));

    // Draw skybox (usually not rotated with the system)
    drawSkybox();
    setLights();

    // ==============================================
    // ROTATE ONLY THE SOLAR SYSTEM
    // ==============================================
    glPushMatrix();
    glScalef(2.0f, 2.0f, 2.0f);

    // Apply 90-degree rotation to solar system only
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);

    // Draw planets and their orbits
    for (int i = 0; i < NUM_PLANETS; i++) {
        if (i != 9 && g_viewOrbit) {
            drawOrbitCircle(g_planet[i].dx, g_planet[i].dy,
                g_planet[i].orbitRadius, g_planet[i].number);
        }
        glColor3f(1.0f, 1.0f, 1.0f);
        drawBody(g_planet[i]);
    }

    // Draw Saturn's ring
    {
        const Body& saturn = g_planet[6];
        glPushMatrix();
        glTranslatef(saturn.x, saturn.y, saturn.z);
        drawRing(g_satRingMesh, saturn.radius, g_tex[10],
            saturn.tilt, saturn.angularVelocity);
        glPopMatrix();
    }

    // Draw Uranus's ring
    {
        const Body& uranus = g_planet[7];
        glPushMatrix();
        glTranslatef(uranus.x, uranus.y, uranus.z);
        drawRing(g_urRingMesh, uranus.radius, 0,
            uranus.tilt, uranus.angularVelocity);
        glPopMatrix();
    }

    // Draw additional scene elements
    if (!g_overview) {
        drawAsteroids();
    }
    drawParticles();

    glPopMatrix();  // END OF SOLAR SYSTEM ROTATION
    // ==============================================

    // Draw stars (outside rotation so they stay fixed)
    drawStars();

    // Draw spaceship WITHOUT rotation (stays in world space)
    if (g_gameMode) {
        renderSpaceship();
    }
}

// =============================================================================
//  ImGui User Interface
// =============================================================================

/**
 * Renders the ImGui control panel with all user controls
 */
static void renderImGui() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Main control panel
    ImGui::SetNextWindowPos({ 10, 10 }, ImGuiCond_Once);
    ImGui::SetNextWindowSize({ 290, 500 }, ImGuiCond_Once);
    ImGui::Begin("Solar System");

    // FPS display
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "FPS: %.1f", g_currentFPS);
    ImGui::Separator();

    // Game Mode Toggle
    renderGameModeControls();

    if (g_gameMode) {
        renderGameModePanel();
    }
    else {
        renderNormalModePanel();
    }

    ImGui::End();

    // Planet Information Panel
    renderPlanetInfoPanel();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

/**
 * Renders the Game Mode toggle button
 */
static void renderGameModeControls() {
    const char* buttonLabel = g_gameMode ? "Exit Game Mode (G)" : "Enter Game Mode (G)";
    if (ImGui::Button(buttonLabel)) {
        toggleGameMode();
    }
}

/**
 * Toggles game mode on/off
 */
static void toggleGameMode() {
    g_gameMode = !g_gameMode;
    if (g_gameMode) {
        // Position ship in front of camera
        glm::vec3 forward = g_camera.forward();
        g_spaceship.position = g_camera.position + forward * 5.0f;
        g_spaceship.speed = 0.0f;
        g_spaceship.yaw = atan2f(forward.x, -forward.z);
        g_spaceship.pitch = -asinf(forward.y);
        // Clamp pitch: -10 to +80 degrees
        g_spaceship.pitch = glm::clamp(g_spaceship.pitch, -0.174f, 1.396f);
        g_followIdx = -1;
    }
    else {
        // Exit game mode - reset camera to default view
        resetCamera();
    }
}

/**
 * Renders the Game Mode control panel
 */
static void renderGameModePanel() {
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "=== GAME MODE ACTIVE ===");
    ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "=== SPACESHIP CONTROLS ===");

    // Speed display
    float maxSpeed = g_spaceship.maxSpeed *
        (g_spaceship.boostActive ? g_spaceship.boostMultiplier : 1.0f);
    ImGui::Text("Speed: %.1f / %.1f", g_spaceship.speed, maxSpeed);

    // Speed bar
    float speedPercent = fabs(g_spaceship.speed) / maxSpeed;
    const char* barLabel = g_spaceship.boostActive ? "BOOST ACTIVE!" : "";
    ImGui::ProgressBar(speedPercent, ImVec2(200, 20), barLabel);

    // Position info
    ImGui::Text("Position: (%.1f, %.1f, %.1f)",
        g_spaceship.position.x, g_spaceship.position.y, g_spaceship.position.z);

    // Orientation info
    float pitchDeg = glm::degrees(g_spaceship.pitch);
    float yawDeg = glm::degrees(g_spaceship.yaw);
    ImGui::Text("Pitch: %.1f°  Yaw: %.1f°", pitchDeg, yawDeg);

    // Controls help - UPDATED
    ImGui::Separator();
    ImGui::TextDisabled("W        - Move Forward");
    ImGui::TextDisabled("S        - Move Backward");
    ImGui::TextDisabled("A/D      - Turn Left/Right");
    ImGui::TextDisabled("Space    - Move Up");
    ImGui::TextDisabled("LCtrl     - Move Down");
    ImGui::TextDisabled("Left Shift - Boost speed");
    ImGui::TextDisabled("G        - Exit Game Mode");
    ImGui::Separator();
}

/**
 * Renders the Normal Mode control panel
 */
static void renderNormalModePanel() {
    // View mode toggle
    ImGui::Text("View Mode");
    ImGui::Separator();
    const char* viewLabel = g_overview ? "Switch to Realistic (Tab)" : "Switch to Overview (Tab)";
    if (ImGui::Button(viewLabel)) {
        toggleViewMode();
    }
    ImGui::Text("Mode: %s", g_overview ? "Overview (scaled)" : "Realistic (true scale)");

    // Orbit toggle
    ImGui::Spacing();
    bool orbits = (g_viewOrbit == 1);
    if (ImGui::Checkbox("Show Orbit Circles (H)", &orbits)) {
        g_viewOrbit = orbits ? 1 : 0;
    }

    // Simulation speed controls
    ImGui::Spacing();
    ImGui::Text("Simulation Speed");
    ImGui::Separator();
    if (ImGui::Button("Speed Up (+)")) {
        adjustSimulationSpeed(2.0f);
    }
    ImGui::SameLine();
    if (ImGui::Button("Slow Down (-)")) {
        adjustSimulationSpeed(0.5f);
    }

    // Planet follow controls
    ImGui::Spacing();
    ImGui::Text("Follow Planet");
    ImGui::Separator();
    renderFollowCombo();

    // Camera controls
    ImGui::Spacing();
    ImGui::Text("Camera Controls");
    ImGui::Separator();
    ImGui::Text("Position: (%.2f, %.2f, %.2f)",
        g_camera.position.x, g_camera.position.y, g_camera.position.z);

    if (ImGui::Button("Reset Camera (Space)")) {
        resetCamera();
    }

    // Controls help
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextDisabled("LMB Drag   - orbit/rotate");
    ImGui::TextDisabled("RMB Drag   - pan");
    ImGui::TextDisabled("Scroll     - zoom");
    ImGui::TextDisabled("WASD       - move/strafe");
    ImGui::TextDisabled("Q/E        - move up/down");
    ImGui::TextDisabled("1-9        - follow planet");
    ImGui::TextDisabled("0          - free camera");
    ImGui::TextDisabled("G          - Game Mode");
}

/**
 * Toggles between overview and realistic view modes
 */
static void toggleViewMode() {
    g_overview = !g_overview;
    // Restore from saved state
    for (int i = 0; i < NUM_PLANETS; i++) {
        g_planet[i] = g_save[i];
    }
    if (g_overview) {
        applyOverviewScale();
    }
    resetCamera();
}

/**
 * Adjusts simulation speed by a multiplier
 */
static void adjustSimulationSpeed(float multiplier) {
    for (int i = 0; i < NUM_PLANETS; i++) {
        g_planet[i].angularVelocity *= multiplier;
        g_planet[i].dr *= multiplier;
    }
}

/**
 * Renders the planet follow combo box
 */
static void renderFollowCombo() {
    const char* names[] = {
        "Free", "Sun", "Mercury", "Venus", "Earth",
        "Mars", "Jupiter", "Saturn", "Uranus", "Neptune", "Moon"
    };
    int sel = g_followIdx + 1;
    if (ImGui::Combo("##follow", &sel, names, 11)) {
        g_followIdx = sel - 1;
        if (g_followIdx >= 0) {
            g_viewOrbit = 0;
            followPlanet(g_followIdx);
        }
        else {
            resetCamera();
        }
    }
}

/**
 * Renders the planet information panel when a planet is selected
 */
static void renderPlanetInfoPanel() {
    if (g_gameMode || g_selectedPlanet < 0 || g_selectedPlanet >= NUM_PLANETS) {
        return;
    }

    ImGui::SetNextWindowPos({ 320, 10 }, ImGuiCond_Once);
    ImGui::SetNextWindowSize({ 360, 320 }, ImGuiCond_Once);
    ImGui::Begin("Planet Information", nullptr, ImGuiWindowFlags_NoCollapse);

    const PlanetInfo& info = g_planetInfo[g_selectedPlanet];

    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "=== %s ===", info.name);
    ImGui::Separator();

    // Basic stats
    ImGui::Text("Distance from Sun: %.2f million km", info.distanceFromSun);
    ImGui::Text("Orbital Speed: %.2f km/s", info.orbitalSpeed);

    float rotPeriod = info.rotationPeriod;
    if (rotPeriod < 0) {
        ImGui::Text("Rotation Period: %.2f days (retrograde)", -rotPeriod);
    }
    else {
        ImGui::Text("Rotation Period: %.2f days", rotPeriod);
    }

    float gravity = info.gravity;
    if (gravity > 0) {
        ImGui::TextColored(ImVec4(0.7f, 1.0f, 0.7f, 1.0f),
            "Surface Gravity: %.2f m/s²", gravity);
    }
    else {
        ImGui::Text("Surface Gravity: N/A");
    }

    // Interesting facts
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.8f, 1.0f, 0.8f, 1.0f), "Interesting Facts:");
    ImGui::TextWrapped("1. %s", info.fact1);
    ImGui::TextWrapped("2. %s", info.fact2);

    // Close button
    ImGui::Spacing();
    ImGui::Separator();
    if (ImGui::Button("Close")) {
        g_selectedPlanet = -1;
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(Click empty space to close)");

    ImGui::End();
}

// =============================================================================
//  GLFW Callbacks
// =============================================================================

/**
 * Mouse scroll callback - handles zoom
 */
void cbScroll(GLFWwindow*, double, double yoff) {
    // Disable zoom in game mode
    if (g_gameMode) {
        return;
    }

    if (g_camera.mode == Camera::Mode::ORBIT) {
        g_camera.orbitDistance *= (1.0f - (float)yoff * 0.1f);
        g_camera.orbitDistance = std::max(0.01f, g_camera.orbitDistance);
        updateOrbitCamera();
    }
    else {
        glm::vec3 forward = g_camera.forward();
        g_camera.position += forward * (float)yoff * g_camera.zoomSpeed;
        g_camera.target = g_camera.position + forward;
    }
}

/**
 * Mouse button callback - handles dragging and planet selection
 */
void cbMouseBtn(GLFWwindow* win, int btn, int action, int) {
    // Don't process if ImGui is capturing mouse
    if (ImGui::GetIO().WantCaptureMouse) {
        g_camera.isDragging = false;
        g_camera.isPanning = false;
        return;
    }

    // Game mode: mouse drag controls ship look
    if (g_gameMode) {
        return;
    }

    // Normal mode: orbit/pan controls
    if (btn == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            g_camera.isDragging = true;
        }
        else if (action == GLFW_RELEASE) {
            g_camera.isDragging = false;
            // Check for planet click (only if no drag occurred)
            double mx, my;
            glfwGetCursorPos(win, &mx, &my);
            int hitIndex;
            if (raycastPlanet((float)mx, (float)my, hitIndex)) {
                g_selectedPlanet = hitIndex;
            }
            else {
                g_selectedPlanet = -1;
            }
        }
    }
    else if (btn == GLFW_MOUSE_BUTTON_RIGHT) {
        g_camera.isPanning = (action == GLFW_PRESS);
        if (action == GLFW_RELEASE) {
            g_camera.isPanning = false;
        }
    }
}

/**
 * Cursor position callback - handles camera rotation and panning
 */
void cbCursorPos(GLFWwindow*, double mx, double my) {
    if (ImGui::GetIO().WantCaptureMouse) {
        g_camera.lastMouseX = mx;
        g_camera.lastMouseY = my;
        return;
    }

    float dx = (float)(mx - g_camera.lastMouseX);
    float dy = (float)(my - g_camera.lastMouseY);
    g_camera.lastMouseX = mx;
    g_camera.lastMouseY = my;

    if (g_gameMode) {
        // Do nothing - mouse doesn't control anything in game mode
        return;
    }

    // Orbit mode rotation
    if (g_camera.isDragging && g_camera.mode == Camera::Mode::ORBIT) {
        g_camera.orbitYaw += dx * g_camera.orbitSpeed * 0.5f;
        g_camera.orbitPitch += dy * g_camera.orbitSpeed * 0.5f;
        g_camera.orbitPitch = glm::clamp(g_camera.orbitPitch, -89.0f, 89.0f);
        updateOrbitCamera();
    }
    // Free mode rotation
    else if (g_camera.isDragging && g_camera.mode == Camera::Mode::FREE) {
        g_camera.yaw += dx * g_camera.orbitSpeed * 0.5f;
        g_camera.pitch += dy * g_camera.orbitSpeed * 0.5f;
        g_camera.pitch = glm::clamp(g_camera.pitch, -89.0f, 89.0f);
        updateFreeCamera();
    }
    // Panning
    else if (g_camera.isPanning) {
        glm::vec3 right = g_camera.right();
        glm::vec3 up = g_camera.up;

        float panScale = g_camera.mode == Camera::Mode::ORBIT ?
            g_camera.orbitDistance * 0.001f : 0.01f;

        glm::vec3 panOffset = -right * dx * panScale + up * dy * panScale;

        if (g_camera.mode == Camera::Mode::ORBIT) {
            g_camera.target += panOffset;
            g_camera.position += panOffset;
        }
        else {
            g_camera.position += panOffset;
            g_camera.target += panOffset;
        }
    }
}

/**
 * Keyboard callback - handles all key inputs
 */
void cbKey(GLFWwindow* win, int key, int, int action, int) {
    if (ImGui::GetIO().WantCaptureKeyboard) {
        return;
    }
    if (action != GLFW_PRESS && action != GLFW_REPEAT) {
        return;
    }

    // --- Game Mode Keys ---
    if (g_gameMode) {
        switch (key) {
        case GLFW_KEY_G:
            g_gameMode = false;
            resetCamera();
            break;
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(win, GLFW_TRUE);
            break;
        default:
            break;
        }
        return;
    }

    // --- Normal Mode Keys ---
    float speed = g_camera.moveSpeed * 0.01f;

    switch (key) {
        // Game mode toggle
    case GLFW_KEY_G:
        toggleGameMode();
        break;

        // Camera movement
    case GLFW_KEY_UP:
    case GLFW_KEY_W:
        if (g_camera.mode == Camera::Mode::FREE) {
            g_camera.position += g_camera.forward() * speed;
            updateFreeCamera();
        }
        else {
            g_camera.orbitDistance *= (1.0f - speed * 0.1f);
            g_camera.orbitDistance = std::max(0.01f, g_camera.orbitDistance);
            updateOrbitCamera();
        }
        break;

    case GLFW_KEY_DOWN:
    case GLFW_KEY_S:
        if (g_camera.mode == Camera::Mode::FREE) {
            g_camera.position -= g_camera.forward() * speed;
            updateFreeCamera();
        }
        else {
            g_camera.orbitDistance *= (1.0f + speed * 0.1f);
            updateOrbitCamera();
        }
        break;

    case GLFW_KEY_LEFT:
    case GLFW_KEY_A:
        if (g_camera.mode == Camera::Mode::FREE) {
            g_camera.position -= g_camera.right() * speed;
            updateFreeCamera();
        }
        else {
            g_camera.orbitYaw += speed * 100.0f;
            updateOrbitCamera();
        }
        break;

    case GLFW_KEY_RIGHT:
    case GLFW_KEY_D:
        if (g_camera.mode == Camera::Mode::FREE) {
            g_camera.position += g_camera.right() * speed;
            updateFreeCamera();
        }
        else {
            g_camera.orbitYaw -= speed * 100.0f;
            updateOrbitCamera();
        }
        break;

    case GLFW_KEY_Q:
        if (g_camera.mode == Camera::Mode::FREE) {
            g_camera.position += g_camera.up * speed;
            updateFreeCamera();
        }
        else {
            g_camera.target.y += speed * 10.0f;
            updateOrbitCamera();
        }
        break;

    case GLFW_KEY_E:
        if (g_camera.mode == Camera::Mode::FREE) {
            g_camera.position -= g_camera.up * speed;
            updateFreeCamera();
        }
        else {
            g_camera.target.y -= speed * 10.0f;
            updateOrbitCamera();
        }
        break;

        // Toggle orbit lines
    case GLFW_KEY_H:
        g_viewOrbit = 1 - g_viewOrbit;
        break;

        // Simulation speed
    case GLFW_KEY_EQUAL:
        adjustSimulationSpeed(2.0f);
        break;

    case GLFW_KEY_MINUS:
        adjustSimulationSpeed(0.5f);
        break;

        // View mode toggle
    case GLFW_KEY_TAB:
        toggleViewMode();
        break;

        // Reset camera
    case GLFW_KEY_SPACE:
        // Only reset camera if NOT in game mode (space is used for vertical movement in game mode)
        if (!g_gameMode) {
            resetCamera();
        }
        break;

        // Follow planet shortcuts
    case GLFW_KEY_0:
        g_followIdx = -1;
        resetCamera();
        break;
    case GLFW_KEY_1: g_followIdx = 1; g_viewOrbit = 0; followPlanet(1); break;
    case GLFW_KEY_2: g_followIdx = 2; g_viewOrbit = 0; followPlanet(2); break;
    case GLFW_KEY_3: g_followIdx = 3; g_viewOrbit = 0; followPlanet(3); break;
    case GLFW_KEY_4: g_followIdx = 4; g_viewOrbit = 0; followPlanet(4); break;
    case GLFW_KEY_5: g_followIdx = 5; g_viewOrbit = 0; followPlanet(5); break;
    case GLFW_KEY_6: g_followIdx = 6; g_viewOrbit = 0; followPlanet(6); break;
    case GLFW_KEY_7: g_followIdx = 7; g_viewOrbit = 0; followPlanet(7); break;
    case GLFW_KEY_8: g_followIdx = 8; g_viewOrbit = 0; followPlanet(8); break;
    case GLFW_KEY_9: g_followIdx = 9; g_viewOrbit = 0; followPlanet(9); break;

        // Exit
    case GLFW_KEY_ESCAPE:
        glfwSetWindowShouldClose(win, GLFW_TRUE);
        break;

    default:
        break;
    }
}

/**
 * Window resize callback
 */
void cbResize(GLFWwindow*, int w, int h) {
    g_winW = w;
    g_winH = h;
}

// =============================================================================
//  Resource Cleanup
// =============================================================================

/**
 * Cleans up all OpenGL resources
 */
static void cleanupResources() {
    // Delete sphere mesh
    glDeleteBuffers(1, &g_sphereMesh.vbo);
    glDeleteBuffers(1, &g_sphereMesh.ebo);
    glDeleteVertexArrays(1, &g_sphereMesh.vao);

    // Delete Saturn ring mesh
    glDeleteBuffers(1, &g_satRingMesh.vbo);
    glDeleteBuffers(1, &g_satRingMesh.ebo);
    glDeleteVertexArrays(1, &g_satRingMesh.vao);

    // Delete Uranus ring mesh
    glDeleteBuffers(1, &g_urRingMesh.vbo);
    glDeleteBuffers(1, &g_urRingMesh.ebo);
    glDeleteVertexArrays(1, &g_urRingMesh.vao);

    // Delete textures
    glDeleteTextures(NUM_TEX, g_tex);
}

// =============================================================================
//  Main Entry Point
// =============================================================================

int main() {
    // Seed random number generator
    srand(static_cast<unsigned>(time(nullptr)));

    // --- Initialize GLFW ---
    if (!glfwInit()) {
        std::cerr << "glfwInit failed\n";
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

    GLFWwindow* window = glfwCreateWindow(g_winW, g_winH,
        "Solar System Simulation - Press G for Game Mode", nullptr, nullptr);
    if (!window) {
        std::cerr << "glfwCreateWindow failed\n";
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // --- Initialize GLEW ---
    if (glewInit() != GLEW_OK) {
        std::cerr << "glewInit failed\n";
        return 1;
    }

    // --- Setup GLFW Callbacks ---
    glfwSetScrollCallback(window, cbScroll);
    glfwSetMouseButtonCallback(window, cbMouseBtn);
    glfwSetCursorPosCallback(window, cbCursorPos);
    glfwSetKeyCallback(window, cbKey);
    glfwSetFramebufferSizeCallback(window, cbResize);

    // --- Initialize ImGui ---
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 120");

    // --- Initialize Scene ---
    initGL();
    loadAllTextures();
    initPlanets();
    initStars();
    initAsteroids();
    initSpaceship();
    if (!g_audio.init()) {
        std::cerr << "Warning: Audio initialization failed\n";
    }
    resetCamera();
    setupPlanetInfo();

    // --- Main Loop ---
    double lastTime = glfwGetTime();
    g_fpsUpdateTime = lastTime;
    const double DT = 1.0 / FPS;
    double accumulator = 0.0;

    while (!glfwWindowShouldClose(window)) {
        // Calculate frame time
        double now = glfwGetTime();
        double frameTime = now - lastTime;
        lastTime = now;

        // Update FPS counter
        g_frameCount++;
        if (now - g_fpsUpdateTime >= 0.5) {
            g_currentFPS = static_cast<float>(g_frameCount / (now - g_fpsUpdateTime));
            g_frameCount = 0;
            g_fpsUpdateTime = now;
        }

        // Process events
        glfwPollEvents();
        g_audio.update();

        // Fixed timestep update
        accumulator += frameTime;
        while (accumulator >= DT) {
            // Update planets (always running)
            updatePlanets();

            // Update spaceship in game mode
            if (g_gameMode) {
                updateSpaceship((float)DT);
            }

            accumulator -= DT;

            // Update follow camera
            if (g_followIdx >= 0 && g_followIdx < NUM_PLANETS && !g_gameMode) {
                g_camera.target = glm::vec3(
                    g_planet[g_followIdx].x,
                    g_planet[g_followIdx].y,
                    g_planet[g_followIdx].z
                );
                updateOrbitCamera();
            }
        }

        // --- Render ---
        glfwGetFramebufferSize(window, &g_winW, &g_winH);
        renderScene();
        renderImGui();
        glfwSwapBuffers(window);
    }

    // --- Cleanup ---
    g_audio.cleanup();
    cleanupResources();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}