#include "solarSystem.h"

// =============================================================================
//  Planet Information Data
// =============================================================================

void setupPlanetInfo() {
    // Sun
    strcpy_s(g_planetInfo[0].name, sizeof(g_planetInfo[0].name), "Sun");
    g_planetInfo[0].distanceFromSun = 0.0f;
    g_planetInfo[0].orbitalSpeed = 0.0f;
    g_planetInfo[0].rotationPeriod = 25.4f;
    g_planetInfo[0].gravity = 274.0f;
    strcpy_s(g_planetInfo[0].fact1, sizeof(g_planetInfo[0].fact1),
        "The Sun accounts for 99.86% of the mass in our solar system.");
    strcpy_s(g_planetInfo[0].fact2, sizeof(g_planetInfo[0].fact2),
        "The Sun's core temperature is about 15 million degrees Celsius.");

    // Mercury
    strcpy_s(g_planetInfo[1].name, sizeof(g_planetInfo[1].name), "Mercury");
    g_planetInfo[1].distanceFromSun = 57.9f;
    g_planetInfo[1].orbitalSpeed = 47.87f;
    g_planetInfo[1].rotationPeriod = 58.6f;
    g_planetInfo[1].gravity = 3.7f;
    strcpy_s(g_planetInfo[1].fact1, sizeof(g_planetInfo[1].fact1),
        "Mercury has no atmosphere and experiences extreme temperature variations.");
    strcpy_s(g_planetInfo[1].fact2, sizeof(g_planetInfo[1].fact2),
        "A day on Mercury (sunrise to sunrise) lasts 176 Earth days.");

    // Venus
    strcpy_s(g_planetInfo[2].name, sizeof(g_planetInfo[2].name), "Venus");
    g_planetInfo[2].distanceFromSun = 108.2f;
    g_planetInfo[2].orbitalSpeed = 35.02f;
    g_planetInfo[2].rotationPeriod = -243.0f;
    g_planetInfo[2].gravity = 8.87f;
    strcpy_s(g_planetInfo[2].fact1, sizeof(g_planetInfo[2].fact1),
        "Venus spins backwards and is the hottest planet due to runaway greenhouse effect.");
    strcpy_s(g_planetInfo[2].fact2, sizeof(g_planetInfo[2].fact2),
        "Venus rotates so slowly that its day is longer than its year.");

    // Earth
    strcpy_s(g_planetInfo[3].name, sizeof(g_planetInfo[3].name), "Earth");
    g_planetInfo[3].distanceFromSun = 149.6f;
    g_planetInfo[3].orbitalSpeed = 29.78f;
    g_planetInfo[3].rotationPeriod = 1.0f;
    g_planetInfo[3].gravity = 9.81f;
    strcpy_s(g_planetInfo[3].fact1, sizeof(g_planetInfo[3].fact1),
        "Earth is the only known planet to support life, with 71% of its surface covered by water.");
    strcpy_s(g_planetInfo[3].fact2, sizeof(g_planetInfo[3].fact2),
        "Earth's atmosphere is composed of 78% nitrogen, 21% oxygen, and trace amounts of other gases.");

    // Mars
    strcpy_s(g_planetInfo[4].name, sizeof(g_planetInfo[4].name), "Mars");
    g_planetInfo[4].distanceFromSun = 227.9f;
    g_planetInfo[4].orbitalSpeed = 24.07f;
    g_planetInfo[4].rotationPeriod = 1.03f;
    g_planetInfo[4].gravity = 3.71f;
    strcpy_s(g_planetInfo[4].fact1, sizeof(g_planetInfo[4].fact1),
        "Mars has the tallest mountain in the solar system: Olympus Mons at 21.9km.");
    strcpy_s(g_planetInfo[4].fact2, sizeof(g_planetInfo[4].fact2),
        "Mars has the largest dust storms in the solar system, lasting for months.");

    // Jupiter
    strcpy_s(g_planetInfo[5].name, sizeof(g_planetInfo[5].name), "Jupiter");
    g_planetInfo[5].distanceFromSun = 778.6f;
    g_planetInfo[5].orbitalSpeed = 13.07f;
    g_planetInfo[5].rotationPeriod = 0.41f;
    g_planetInfo[5].gravity = 24.79f;
    strcpy_s(g_planetInfo[5].fact1, sizeof(g_planetInfo[5].fact1),
        "Jupiter's Great Red Spot is a storm larger than Earth that has raged for hundreds of years.");
    strcpy_s(g_planetInfo[5].fact2, sizeof(g_planetInfo[5].fact2),
        "Jupiter has at least 95 known moons, including the largest moon in the solar system, Ganymede.");

    // Saturn
    strcpy_s(g_planetInfo[6].name, sizeof(g_planetInfo[6].name), "Saturn");
    g_planetInfo[6].distanceFromSun = 1433.5f;
    g_planetInfo[6].orbitalSpeed = 9.68f;
    g_planetInfo[6].rotationPeriod = 0.44f;
    g_planetInfo[6].gravity = 10.44f;
    strcpy_s(g_planetInfo[6].fact1, sizeof(g_planetInfo[6].fact1),
        "Saturn's density is less than water - it would float if placed in a giant bathtub.");
    strcpy_s(g_planetInfo[6].fact2, sizeof(g_planetInfo[6].fact2),
        "Saturn's rings are made of ice and rock particles, some as small as dust grains.");

    // Uranus
    strcpy_s(g_planetInfo[7].name, sizeof(g_planetInfo[7].name), "Uranus");
    g_planetInfo[7].distanceFromSun = 2872.5f;
    g_planetInfo[7].orbitalSpeed = 6.80f;
    g_planetInfo[7].rotationPeriod = -0.72f;
    g_planetInfo[7].gravity = 8.69f;
    strcpy_s(g_planetInfo[7].fact1, sizeof(g_planetInfo[7].fact1),
        "Uranus rotates on its side, likely from a massive ancient collision.");
    strcpy_s(g_planetInfo[7].fact2, sizeof(g_planetInfo[7].fact2),
        "Uranus has the coldest atmosphere of any planet, reaching -224°C.");

    // Neptune
    strcpy_s(g_planetInfo[8].name, sizeof(g_planetInfo[8].name), "Neptune");
    g_planetInfo[8].distanceFromSun = 4495.1f;
    g_planetInfo[8].orbitalSpeed = 5.43f;
    g_planetInfo[8].rotationPeriod = 0.67f;
    g_planetInfo[8].gravity = 11.15f;
    strcpy_s(g_planetInfo[8].fact1, sizeof(g_planetInfo[8].fact1),
        "Neptune has the strongest winds in the solar system, reaching 2,100 km/h.");
    strcpy_s(g_planetInfo[8].fact2, sizeof(g_planetInfo[8].fact2),
        "Neptune is the most distant planet and was the first located through mathematical predictions.");

    // Moon
    strcpy_s(g_planetInfo[9].name, sizeof(g_planetInfo[9].name), "Moon");
    g_planetInfo[9].distanceFromSun = 149.6f;
    g_planetInfo[9].orbitalSpeed = 1.02f;
    g_planetInfo[9].rotationPeriod = 27.3f;
    g_planetInfo[9].gravity = 1.62f;
    strcpy_s(g_planetInfo[9].fact1, sizeof(g_planetInfo[9].fact1),
        "The Moon is Earth's only natural satellite and is the fifth-largest moon.");
    strcpy_s(g_planetInfo[9].fact2, sizeof(g_planetInfo[9].fact2),
        "The Moon is moving away from Earth at about 3.8 cm per year.");

    // Planet X (Hypothetical)
    strcpy_s(g_planetInfo[10].name, sizeof(g_planetInfo[10].name), "Planet X");
    g_planetInfo[10].distanceFromSun = 0.0f;
    g_planetInfo[10].orbitalSpeed = 0.0f;
    g_planetInfo[10].rotationPeriod = 0.0f;
    g_planetInfo[10].gravity = 0.0f;
    strcpy_s(g_planetInfo[10].fact1, sizeof(g_planetInfo[10].fact1),
        "A hypothetical planet beyond Neptune that may explain orbital anomalies.");
    strcpy_s(g_planetInfo[10].fact2, sizeof(g_planetInfo[10].fact2),
        "Planet X is estimated to be 5-10 times the mass of Earth if it exists.");
}