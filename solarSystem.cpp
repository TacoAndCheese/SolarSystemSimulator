#include "solarSystem.h"
#include "spaceship.h"

// =============================================================================
//  Global Variable Definitions
// =============================================================================

// Camera
Camera g_camera;

// Celestial bodies
Body g_planet[NUM_PLANETS];
Body g_save[NUM_PLANETS];
Body g_asteroids[AST_COUNT];
StarPoint g_stars[STAR_COUNT];

// Particle system
std::vector<Particle> g_particles;

// Textures and meshes
GLuint g_tex[NUM_TEX] = {};
Mesh g_sphereMesh;
Mesh g_satRingMesh;
Mesh g_urRingMesh;

// State
int g_followIdx = -1;
bool g_overview = true;
int g_viewOrbit = 1;
int g_selectedPlanet = -1;

// Window
int g_winW = 1280;
int g_winH = 720;
float g_currentFPS = 0.0f;
float g_lightIntensity = 2.0f;  

// Planet info
PlanetInfo g_planetInfo[NUM_PLANETS];

// =============================================================================
//  Camera Method Implementations
// =============================================================================

glm::vec3 Camera::forward() const {
    if (mode == Mode::ORBIT) {
        return glm::normalize(target - position);
    }
    // Free mode: calculate forward from yaw and pitch
    return glm::normalize(glm::vec3(
        -sin(glm::radians(yaw)) * cos(glm::radians(pitch)),
        -sin(glm::radians(pitch)),
        -cos(glm::radians(yaw)) * cos(glm::radians(pitch))
    ));
}

glm::vec3 Camera::right() const {
    return glm::normalize(glm::cross(forward(), up));
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(position, target, up);
}

glm::mat4 Camera::getProjectionMatrix(float aspect, float fov,
    float nearPlane, float farPlane) const {
    return glm::perspective(glm::radians(fov), aspect, nearPlane, farPlane);
}

// =============================================================================
//  Helper Functions
// =============================================================================

/** Generate random float in range [min, max] */
static float randF(float min, float max) {
    return min + static_cast<float>(rand()) / static_cast<float>(RAND_MAX / (max - min));
}

// =============================================================================
//  Mesh Construction
// =============================================================================

/**
 * Build a UV sphere mesh with specified subdivision levels
 */
Mesh buildSphereMesh(int stacks, int slices) {
    std::vector<Vertex> verts;
    std::vector<unsigned int> idx;

    // Generate vertices
    for (int stack = 0; stack <= stacks; ++stack) {
        float phi = PI * stack / stacks;
        float sinPhi = sinf(phi);
        float cosPhi = cosf(phi);

        for (int slice = 0; slice <= slices; ++slice) {
            float theta = 2.0f * PI * slice / slices;
            float sinTheta = sinf(theta);
            float cosTheta = cosf(theta);

            Vertex v;
            // Normal = position for a unit sphere
            v.nx = sinPhi * cosTheta;
            v.ny = sinPhi * sinTheta;
            v.nz = cosPhi;
            v.px = v.nx;
            v.py = v.ny;
            v.pz = v.nz;
            v.u = (float)slice / slices;
            v.v = (float)stack / stacks;
            verts.push_back(v);
        }
    }

    // Generate triangle indices
    for (int stack = 0; stack < stacks; ++stack) {
        for (int slice = 0; slice < slices; ++slice) {
            unsigned int a = stack * (slices + 1) + slice;
            unsigned int b = a + (slices + 1);

            // Two triangles per quad
            idx.push_back(a);
            idx.push_back(b);
            idx.push_back(a + 1);
            idx.push_back(b);
            idx.push_back(b + 1);
            idx.push_back(a + 1);
        }
    }

    // Create OpenGL buffers
    Mesh mesh;
    mesh.indexCount = (int)idx.size();

    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glGenBuffers(1, &mesh.ebo);

    glBindVertexArray(mesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex),
        verts.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(unsigned int),
        idx.data(), GL_STATIC_DRAW);

    // Setup legacy OpenGL client-side arrays
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glBindVertexArray(0);

    return mesh;
}

/**
 * Build a ring/disc mesh with inner and outer radius
 */
Mesh buildDiskMesh(float innerR, float outerR, int slices) {
    std::vector<Vertex> verts;
    std::vector<unsigned int> idx;

    // Generate vertices (two per slice: outer and inner)
    for (int slice = 0; slice <= slices; ++slice) {
        float theta = 2.0f * PI * slice / slices;
        float c = cosf(theta);
        float s = sinf(theta);

        // Outer vertex
        Vertex outer;
        outer.px = outerR * c;
        outer.py = outerR * s;
        outer.pz = 0;
        outer.nx = 0;
        outer.ny = 0;
        outer.nz = 1;
        outer.u = (float)slice / slices;
        outer.v = 1.0f;
        verts.push_back(outer);

        // Inner vertex
        Vertex inner;
        inner.px = innerR * c;
        inner.py = innerR * s;
        inner.pz = 0;
        inner.nx = 0;
        inner.ny = 0;
        inner.nz = 1;
        inner.u = (float)slice / slices;
        inner.v = 0.0f;
        verts.push_back(inner);
    }

    // Generate triangle indices
    for (int slice = 0; slice < slices; ++slice) {
        unsigned int o0 = slice * 2;
        unsigned int i0 = slice * 2 + 1;
        unsigned int o1 = slice * 2 + 2;
        unsigned int i1 = slice * 2 + 3;

        idx.push_back(o0);
        idx.push_back(i0);
        idx.push_back(o1);
        idx.push_back(i0);
        idx.push_back(i1);
        idx.push_back(o1);
    }

    // Create OpenGL buffers
    Mesh mesh;
    mesh.indexCount = (int)idx.size();

    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glGenBuffers(1, &mesh.ebo);

    glBindVertexArray(mesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex),
        verts.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(unsigned int),
        idx.data(), GL_STATIC_DRAW);
    glBindVertexArray(0);

    return mesh;
}

/**
 * Draw a mesh using legacy OpenGL immediate-mode compatible calls
 */
void drawMesh(const Mesh& mesh) {
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    glVertexPointer(3, GL_FLOAT, sizeof(Vertex), (void*)offsetof(Vertex, px));
    glNormalPointer(GL_FLOAT, sizeof(Vertex), (void*)offsetof(Vertex, nx));
    glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), (void*)offsetof(Vertex, u));

    glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, nullptr);

    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

// =============================================================================
//  GLM Camera Helpers
// =============================================================================

void applyPerspective(float fovDeg, float aspect, float near_, float far_) {
    glm::mat4 proj = glm::perspective(glm::radians(fovDeg), aspect, near_, far_);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(glm::value_ptr(proj));
}

// =============================================================================
//  Texture Loading
// =============================================================================

/**
 * Load a PNG image as an OpenGL texture
 * Returns 0 if loading fails
 */
static GLuint loadPNG(const char* path) {
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &width, &height, &channels, 0);

    if (!data) {
        std::cerr << "[WARN] Texture not found: " << path << "\n";
        return 0;
    }

    GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, (GLint)format, width, height, 0,
        format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    stbi_image_free(data);
    return textureID;
}

void loadAllTextures() {
    const std::string basePath = "./Assets/solar_system_tex/";

    // Planet textures (index matches planet number)
    const char* planetTex[10] = {
        "8k_sun.jpg",           // 0: Sun
        "8k_mercury.jpg",       // 1: Mercury
        "8k_venus_surface.jpg", // 2: Venus
        "8k_earth_daymap.jpg",  // 3: Earth
        "8k_mars.jpg",          // 4: Mars
        "8k_jupiter.jpg",       // 5: Jupiter
        "8k_saturn.jpg",        // 6: Saturn
        "2k_uranus.jpg",        // 7: Uranus
        "2k_neptune.jpg",       // 8: Neptune
        "8k_moon.jpg"           // 9: Moon
    };

    for (int i = 0; i < 10; i++) {
        g_tex[i] = loadPNG((basePath + planetTex[i]).c_str());
    }

    // Saturn's ring texture
    g_tex[10] = loadPNG((basePath + "8k_saturn_ring_alpha.png").c_str());

    // Skybox textures (all use same Milky Way image for all faces)
    const char* skyTex = "8k_stars_milky_way.jpg";
    for (int i = 0; i < 6; i++) {
        g_tex[11 + i] = loadPNG((basePath + skyTex).c_str());
    }
}

// =============================================================================
//  Planet Initialization
// =============================================================================

void initPlanets() {
    // Helper lambda for cleaner initialization
    auto setPlanet = [](int idx, float orbitRadius, float radius,
        float angularVel, int texIdx, float rotSpeed,
        float tiltAngle, float biasVal, float initAngle = 0.0f,
        float dx = 0.0f, float dy = 0.0f) {
            g_planet[idx].orbitRadius = orbitRadius;
            g_planet[idx].radius = radius;
            g_planet[idx].angularVelocity = angularVel;
            g_planet[idx].number = texIdx;
            g_planet[idx].dr = rotSpeed;
            g_planet[idx].tilt = tiltAngle;
            g_planet[idx].bias = biasVal;
            g_planet[idx].angle = initAngle;
            g_planet[idx].dx = dx;
            g_planet[idx].dy = dy;
            g_planet[idx].r = g_planet[idx].g = g_planet[idx].b = 1.0f;
        };

    // Sun (index 0)
    setPlanet(0, 0, 1.0f, 0.01f, 0, BASE_ZZ / 25.4f, 0, 0);

    // Mercury (index 1)
    setPlanet(1, BASE_R * 0.3871f, BASE * 0.382f, BASE_GZ * 365.0f / 88.0f,
        1, BASE_ZZ / 59.0f, 0, 28);

    // Venus (index 2)
    setPlanet(2, BASE_R * 0.7233f, BASE * 0.949f, BASE_GZ * 365.0f / 224.0f,
        2, BASE_ZZ / (-243.0f), 177, 12);

    // Earth (index 3)
    setPlanet(3, BASE_R, BASE, BASE_GZ, 3, BASE_ZZ, 23, 12);

    // Mars (index 4)
    setPlanet(4, BASE_R * 1.5237f, BASE * 0.532f, BASE_GZ * 365.0f / 687.0f,
        4, BASE_ZZ, 25, 21);

    // Jupiter (index 5)
    setPlanet(5, BASE_R * 5.2026f, BASE * 11.209f, BASE_GZ / 11.86f,
        5, BASE_ZZ * 24.0f / 10.0f, 3, 3);

    // Saturn (index 6)
    setPlanet(6, BASE_R * 9.5549f, BASE * 9.449f, BASE_GZ / 29.5f,
        6, BASE_ZZ * 24.0f / 10.5f, 30, 5, toRad(-30));

    // Uranus (index 7)
    setPlanet(7, BASE_R * 19.2184f, BASE * 4.007f, BASE_GZ / 84.0f,
        7, BASE_ZZ * 24.0f / 17.0f, 98, 7, toRad(-60));

    // Neptune (index 8)
    setPlanet(8, BASE_R * 30.1104f, BASE * 3.883f, BASE_GZ / 164.82f,
        8, BASE_ZZ * 24.0f / 16.0f, 30, 7, toRad(-90));

    // Moon (index 9 - orbits Earth)
    setPlanet(9, BASE * 1.5f, BASE * 0.273f, BASE_GZ * 365.0f / 27.0f,
        9, BASE_ZZ / 27.0f, 0, 40);

    // Planet X (index 10 - hypothetical)
    setPlanet(10, BASE_R * 1.5f, BASE * 0.3f, BASE_GZ / 2.82f,
        -1, BASE_ZZ * 24.0f / 16.0f, 30, 10, 0, 2.0f, 2.0f);

    // Save initial state for view switching
    for (int i = 0; i < NUM_PLANETS; i++) {
        g_save[i] = g_planet[i];
    }
}

// =============================================================================
//  Stars and Asteroids Initialization
// =============================================================================

void initStars() {
    // Generate random stars on a sphere of radius 100
    for (int i = 0; i < STAR_COUNT; i++) {
        float theta = randF(0.0f, PI);
        float phi = randF(0.0f, 2.0f * PI);
        g_stars[i] = {
            100.0f * sinf(theta) * cosf(phi),
            100.0f * sinf(theta) * sinf(phi),
            100.0f * cosf(theta)
        };
    }
}

void initAsteroids() {
    // Asteroids orbit between Mars and Jupiter
    float inner = g_planet[4].orbitRadius;
    float outer = g_planet[5].orbitRadius;

    std::mt19937 gen;
    std::normal_distribution<float> dist((inner + outer - 5.0f) / 2.0f, 1.0f);

    for (int i = 0; i < AST_COUNT; i++) {
        Body& ast = g_asteroids[i];
        ast.orbitRadius = dist(gen);

        // Clamp to reasonable range
        ast.orbitRadius = std::max(ast.orbitRadius, inner + 0.1f);
        if (ast.orbitRadius > 1000.0f) {
            ast.orbitRadius = (outer - inner) / 10.0f;
        }

        ast.angularVelocity = BASE_GZ * 365.0f / (800.0f + rand() % 500);
        ast.angle = randF(0.0f, 2.0f * PI);
        ast.radius = (1.0f + rand() % 20) / 10.0f;
        ast.x = ast.orbitRadius * cosf(ast.angle);
        ast.y = ast.orbitRadius * sinf(ast.angle);
        ast.z = (rand() % 8) / 10.0f;
    }
}

// =============================================================================
//  OpenGL Initialization
// =============================================================================

void initGL() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT1);
    glEnable(GL_LIGHT2);
    glEnable(GL_LIGHT3);
    glEnable(GL_LIGHT4);
    glEnable(GL_LIGHT5);
    glEnable(GL_LIGHT6);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    glShadeModel(GL_SMOOTH);

    // Build meshes
    g_sphereMesh = buildSphereMesh(60, 60);
    g_satRingMesh = buildDiskMesh(1.3f, 2.2f, 80);
    g_urRingMesh = buildDiskMesh(1.3f, 1.35f, 80);
}

// =============================================================================
//  Lighting Setup
// =============================================================================

void setLights() {
    GLfloat sunPos[] = { 0, 0, 0, 1 };
    glLightfv(GL_LIGHT0, GL_POSITION, sunPos);

    // Apply intensity multiplier
    float intensity = g_lightIntensity;

    GLfloat ambient[] = { 0.5f * intensity, 0.5f * intensity, 0.5f * intensity, 1.0f };
    GLfloat diffuse[] = { 1.0f * intensity, 1.0f * intensity, 1.0f * intensity, 1.0f };
    GLfloat specular[] = { 1.0f * intensity, 1.0f * intensity, 1.0f * intensity, 1.0f };
    GLfloat white[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat shininess[] = { 100.0f };

    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular);

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, white);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, white);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, shininess);

    // Directional lights
    GLfloat positions[6][4] = {
        { 2, 0, 0, 1 }, { -2, 0, 0, 1 },
        { 0, 2, 0, 1 }, { 0, -2, 0, 1 },
        { 0, 0, 2, 1 }, { 0, 0, -2, 1 }
    };
    GLenum lights[6] = { GL_LIGHT1, GL_LIGHT2, GL_LIGHT3,
                         GL_LIGHT4, GL_LIGHT5, GL_LIGHT6 };

    for (int i = 0; i < 6; i++) {
        glLightfv(lights[i], GL_POSITION, positions[i]);
        glLightfv(lights[i], GL_DIFFUSE, white);
        GLfloat dirAmbient[] = { 0.5f * intensity, 0.5f * intensity,
                                 0.5f * intensity, 1.0f };
        glLightfv(lights[i], GL_AMBIENT, dirAmbient);
    }

    // Reset material for planets
    GLfloat noMat[] = { 0, 0, 0, 1 };
    GLfloat matAmb[] = { 0.7f, 0.7f, 0.7f, 1.0f };
    glMaterialfv(GL_FRONT, GL_AMBIENT, matAmb);
    glMaterialfv(GL_FRONT, GL_SPECULAR, noMat);
    glMaterialfv(GL_FRONT, GL_EMISSION, noMat);
}

// =============================================================================
//  Rendering Functions
// =============================================================================

void drawSphere(float radius, GLuint texID, float r, float g, float b,
    float tilt, float rotation) {
    glPushMatrix();
    glRotatef(tilt, 1, 0, 0);
    glRotatef(rotation, 0, 0, 1);
    glScalef(radius, radius, radius);

    if (texID) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texID);
    }
    else {
        glDisable(GL_TEXTURE_2D);
    }

    glColor3f(r, g, b);
    drawMesh(g_sphereMesh);
    glDisable(GL_TEXTURE_2D);
    glPopMatrix();
}

void drawBody(const Body& body) {
    glPushMatrix();
    glTranslatef(body.x, body.y, body.z);
    GLuint tex = (body.number >= 0 && body.number < NUM_TEX) ? g_tex[body.number] : 0;
    drawSphere(body.radius, tex, body.r, body.g, body.b, body.tilt, body.rotation);
    glPopMatrix();
}

void drawRing(const Mesh& ringMesh, float planetRadius, GLuint texID,
    float tilt, float rotAngle) {
    glPushMatrix();
    glRotatef(rotAngle, 0, 0, 1);
    glRotatef(tilt, 1, 0, 0);
    glScalef(planetRadius, planetRadius, planetRadius);

    if (texID) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texID);
    }

    glColor3f(1, 1, 1);
    glDisable(GL_LIGHTING);
    drawMesh(ringMesh);
    glEnable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glPopMatrix();
}

void drawOrbitCircle(float cx, float cy, float R, int number) {
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    // Grey for Planet X, white for others
    glColor4f((number == -1) ? 0.5f : 1.0f,
        (number == -1) ? 0.5f : 1.0f,
        (number == -1) ? 0.5f : 1.0f, 1.0f);

    glBegin(GL_LINE_LOOP);
    const int SEGMENTS = 1000;
    for (int i = 0; i < SEGMENTS; i++) {
        float angle = i * 2.0f * PI / SEGMENTS;
        glVertex3f(cx + R * cosf(angle), cy + R * sinf(angle), 0);
    }
    glEnd();

    glEnable(GL_LIGHTING);
}

void drawSkybox() {
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);

    glPushMatrix();

    // Six faces of the skybox cube
    struct Face { int texIdx; float verts[4][3]; float uvs[4][2]; };
    Face faces[6] = {
        // Right (+X)
        {11, {{ 100,-100,-100}, { 100, 100,-100}, { 100, 100, 100}, { 100,-100, 100}},
         {{1,0}, {0,0}, {0,1}, {1,1}}},
         // Left (-X)
         {12, {{-100,-100,-100}, {-100, 100,-100}, {-100, 100, 100}, {-100,-100, 100}},
          {{1,0}, {0,0}, {0,1}, {1,1}}},
          // Top (+Y)
          {13, {{-100, 100,-100}, { 100, 100,-100}, { 100, 100, 100}, {-100, 100, 100}},
           {{0,0}, {1,0}, {1,1}, {0,1}}},
           // Bottom (-Y)
           {14, {{-100,-100,-100}, { 100,-100,-100}, { 100,-100, 100}, {-100,-100, 100}},
            {{1,0}, {0,0}, {0,1}, {1,1}}},
            // Front (+Z)
            {15, {{-100,-100, 100}, { 100,-100, 100}, { 100, 100, 100}, {-100, 100, 100}},
             {{0,0}, {1,0}, {1,1}, {0,1}}},
             // Back (-Z)
             {16, {{-100,-100,-100}, { 100,-100,-100}, { 100, 100,-100}, {-100, 100,-100}},
              {{1,0}, {0,0}, {0,1}, {1,1}}}
    };

    glColor3f(1, 1, 1);
    for (const Face& face : faces) {
        if (g_tex[face.texIdx]) {
            glBindTexture(GL_TEXTURE_2D, g_tex[face.texIdx]);
        }
        glBegin(GL_QUADS);
        for (int v = 0; v < 4; v++) {
            glTexCoord2f(face.uvs[v][0], face.uvs[v][1]);
            glVertex3f(face.verts[v][0], face.verts[v][1], face.verts[v][2]);
        }
        glEnd();
    }

    glPopMatrix();
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

void drawStars() {
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_POINT_SMOOTH);
    glPointSize(2.0f);

    glBegin(GL_POINTS);
    glColor3f(0.8f, 0.8f, 1.0f);
    for (int i = 0; i < STAR_COUNT; i++) {
        glVertex3f(g_stars[i].x, g_stars[i].y, g_stars[i].z);
    }
    glEnd();

    glEnable(GL_LIGHTING);
}

void drawAsteroids() {
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor3f(1, 1, 0.8f);

    glBegin(GL_POINTS);
    for (int i = 0; i < AST_COUNT; i++) {
        glVertex3f(g_asteroids[i].x, g_asteroids[i].y, g_asteroids[i].z);
    }
    glEnd();

    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

void drawParticles() {
    if (g_particles.empty()) return;

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPointSize(3.0f);

    glBegin(GL_POINTS);
    glColor3f(1, 1, 1);
    for (const Particle& p : g_particles) {
        glVertex3f(p.x, p.y, p.z);
    }
    glEnd();

    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

// =============================================================================
//  Planet Selection (Raycasting)
// =============================================================================

bool raycastPlanet(float mouseX, float mouseY, int& hitIndex) {
    // Convert mouse coordinates to NDC [-1, 1]
    float ndcX = (2.0f * mouseX) / g_winW - 1.0f;
    float ndcY = 1.0f - (2.0f * mouseY) / g_winH;

    // Get projection and view matrices
    float aspect = g_winH > 0 ? (float)g_winW / (float)g_winH : 1.0f;
    glm::mat4 proj = glm::perspective(glm::radians(60.0f), aspect, 0.01f, 2000.0f);
    glm::mat4 view = g_camera.getViewMatrix();

    // Calculate ray direction in world space
    glm::vec4 rayClip(ndcX, ndcY, -1.0f, 1.0f);
    glm::vec4 rayEye = glm::inverse(proj) * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
    glm::vec3 rayDir = glm::normalize(glm::vec3(glm::inverse(view) * rayEye));
    glm::vec3 rayOrigin = g_camera.position;

    // Test intersection with each planet's bounding sphere
    float minDist = FLT_MAX;
    hitIndex = -1;

    for (int i = 0; i < NUM_PLANETS; i++) {
        glm::vec3 center(g_planet[i].x, g_planet[i].y, g_planet[i].z);
        float radius = g_planet[i].radius;

        // Solve quadratic for ray-sphere intersection
        glm::vec3 oc = rayOrigin - center;
        float a = glm::dot(rayDir, rayDir);
        float b = 2.0f * glm::dot(oc, rayDir);
        float c = glm::dot(oc, oc) - radius * radius;
        float disc = b * b - 4.0f * a * c;

        if (disc >= 0) {
            float t = (-b - sqrtf(disc)) / (2.0f * a);
            if (t > 0 && t < minDist) {
                minDist = t;
                hitIndex = i;
            }
        }
    }

    return hitIndex != -1;
}

// =============================================================================
//  Particle System
// =============================================================================

void emitParticle() {
    const Body& hx = g_planet[10];  // Planet X
    float dist = sqrtf(hx.x * hx.x + hx.y * hx.y + hx.z * hx.z) + 0.001f;

    Particle p;
    p.x = hx.x;
    p.y = hx.y;
    p.z = hx.z;
    // Move outward from center
    p.vx = hx.x / (dist * 60.0f);
    p.vy = hx.y / (dist * 60.0f);
    p.vz = hx.z / (dist * 60.0f);
    p.life = 0.5f / dist;
    g_particles.push_back(p);
}

void updateParticles() {
    for (Particle& p : g_particles) {
        p.x += p.vx;
        p.y += p.vy;
        p.z += p.vz;
        p.life -= 0.01f;
    }

    // Remove dead particles
    g_particles.erase(
        std::remove_if(g_particles.begin(), g_particles.end(),
            [](const Particle& p) { return p.life <= 0; }),
        g_particles.end());
}

// =============================================================================
//  Simulation Update
// =============================================================================

void updatePlanets() {
    const int FOLLOW_OFFSET = 1;  // Visual offset for orbit trails

    for (int i = 0; i < NUM_PLANETS; i++) {
        // Update self-rotation
        g_planet[i].rotation += g_planet[i].dr;

        if (i == 9) {
            // Moon: orbits Earth (index 3)
            g_planet[i].angle += g_planet[i].angularVelocity;
            g_planet[i].x = g_planet[3].x + g_planet[i].orbitRadius * cosf(g_planet[i].angle);
            g_planet[i].y = g_planet[3].y + g_planet[i].orbitRadius * sinf(g_planet[i].angle);

            // Orbit trail offset
            float ca = (g_planet[i].orbitRadius + FOLLOW_OFFSET * g_planet[i].radius * g_planet[i].bias) * cosf(g_planet[i].angle);
            float sa = (g_planet[i].orbitRadius + FOLLOW_OFFSET * g_planet[i].radius * g_planet[i].bias) * sinf(g_planet[i].angle);
            g_planet[i].cx = g_planet[3].x + ca;
            g_planet[i].cy = g_planet[3].y + sa;
            g_planet[i].cz = g_planet[i].radius;
        }
        else {
            // Planet: orbits the sun
            g_planet[i].angle += g_planet[i].angularVelocity;
            g_planet[i].x = g_planet[i].orbitRadius * cosf(g_planet[i].angle) + g_planet[i].dx;
            g_planet[i].y = g_planet[i].orbitRadius * sinf(g_planet[i].angle) + g_planet[i].dy;

            // Orbit trail offset (ahead of the planet)
            float nextAngle = g_planet[i].angle + g_planet[i].angularVelocity * 10.0f;
            g_planet[i].cx = (g_planet[i].orbitRadius + FOLLOW_OFFSET * g_planet[i].radius * g_planet[i].bias) * cosf(nextAngle);
            g_planet[i].cy = (g_planet[i].orbitRadius + FOLLOW_OFFSET * g_planet[i].radius * g_planet[i].bias) * sinf(nextAngle);
            g_planet[i].cz = 0;
        }
    }

    // Update asteroids
    for (int i = 0; i < AST_COUNT; i++) {
        g_asteroids[i].angle += g_asteroids[i].angularVelocity;
        g_asteroids[i].x = g_asteroids[i].orbitRadius * cosf(g_asteroids[i].angle);
        g_asteroids[i].y = g_asteroids[i].orbitRadius * sinf(g_asteroids[i].angle);
    }

    // Emit particles from Planet X
    emitParticle();
    updateParticles();
}

// =============================================================================
//  View Scale Switching
// =============================================================================

void applyOverviewScale() {
    // Scale orbits to be visually distinct in overview mode
    float base = 5.0f;
    for (int i = 1; i < NUM_PLANETS; i++) {
        g_planet[i].orbitRadius = base * (0.2f + i * 0.2f);
    }

    // Scale planet sizes for visibility
    float radii[NUM_PLANETS] = {
        base * 0.2f,  // Sun
        base * 0.05f, // Mercury
        base * 0.05f, // Venus
        base * 0.06f, // Earth
        base * 0.06f, // Mars
        base * 0.15f, // Jupiter
        base * 0.1f,  // Saturn
        base * 0.08f, // Uranus
        base * 0.07f, // Neptune
        base * 0.02f, // Moon
        base * 0.03f  // Planet X
    };

    float biases[NUM_PLANETS] = {
        0, 5, 5, 4.4f, 4.45f, 2.35f, 4.25f, 4.1f, 3.9f, 11.5f, 10.0f
    };

    for (int i = 0; i < NUM_PLANETS; i++) {
        g_planet[i].radius = radii[i];
        g_planet[i].bias = biases[i];
    }

    g_planet[9].orbitRadius = 0.5f;   // Moon
    g_planet[10].orbitRadius = 4.5f;  // Planet X
}

// =============================================================================
//  Camera Update Functions
// =============================================================================

void updateOrbitCamera() {
    float pitchRad = glm::radians(g_camera.orbitPitch);
    float yawRad = glm::radians(g_camera.orbitYaw);

    glm::vec3 offset;
    offset.x = g_camera.orbitDistance * cosf(pitchRad) * sinf(yawRad);
    offset.y = g_camera.orbitDistance * sinf(pitchRad);
    offset.z = g_camera.orbitDistance * cosf(pitchRad) * cosf(yawRad);

    g_camera.position = g_camera.target + offset;
    g_camera.up = glm::vec3(0.0f, 1.0f, 0.0f);
}

void updateFreeCamera() {
    g_camera.up = glm::vec3(0.0f, 1.0f, 0.0f);

    glm::vec3 forward;
    forward.x = -sinf(glm::radians(g_camera.yaw)) * cosf(glm::radians(g_camera.pitch));
    forward.y = -sinf(glm::radians(g_camera.pitch));
    forward.z = -cosf(glm::radians(g_camera.yaw)) * cosf(glm::radians(g_camera.pitch));

    g_camera.target = g_camera.position + glm::normalize(forward);
}

void resetCamera() {
    g_camera.position = glm::vec3(-5.0f, 5.0f, 6.0f);
    g_camera.target = glm::vec3(0.0f, 0.0f, 0.0f);
    g_camera.orbitDistance = glm::distance(g_camera.position, g_camera.target);
    g_camera.orbitPitch = 0.0f;
    g_camera.orbitYaw = 0.0f;
    g_camera.yaw = 0.0f;
    g_camera.pitch = 0.0f;
    g_camera.mode = Camera::Mode::ORBIT;
}

void followPlanet(int idx) {
    if (idx >= 0 && idx < NUM_PLANETS) {
        g_camera.target = glm::vec3(g_planet[idx].x, g_planet[idx].y, g_planet[idx].z);
        g_camera.mode = Camera::Mode::ORBIT;
        updateOrbitCamera();
    }
}