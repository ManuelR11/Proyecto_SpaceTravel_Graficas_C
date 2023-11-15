#include <SDL.h>
#include <SDL_events.h>
#include <iostream>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <sstream>
#include <vector>
#include "color.h"
#include "framebuffer.h"
#include "uniforms.h"
#include "shaders.h"
#include "fragment.h"
#include "triangle.h"
#include "camera.h"
#include "ObjLoader.h"
#include "noise.h"
#include <unordered_map>


SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
Color currentColor;

bool init() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "Error: Fallo en la inicializacion SDL: " << SDL_GetError() << std::endl;
        return false;
    }

    window = SDL_CreateWindow("Software Renderer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Error: Fallo en la creacion SDL window: " << SDL_GetError() << std::endl;
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Error: Fallo en la creacion SDL renderer: " << SDL_GetError() << std::endl;
        return false;
    }

    setupNoise();

    return true;
}

void setColor(const Color& color) {
    currentColor = color;
}

void render(const std::vector<glm::vec3>& VBO, const Uniforms& uniforms) {
    std::vector<Vertex> transformedVertices(VBO.size() / 3);
    for (size_t i = 0; i < VBO.size() / 3; ++i) {
        Vertex vertex = { VBO[i * 3], VBO[i * 3 + 1], VBO[i * 3 + 2] };
        transformedVertices[i] = vertexShader(vertex, uniforms);
    }
    std::vector<std::vector<Vertex>> assembledVertices(transformedVertices.size() / 3);
    for (size_t i = 0; i < transformedVertices.size() / 3; ++i) {
        Vertex edge1 = transformedVertices[3 * i];
        Vertex edge2 = transformedVertices[3 * i + 1];
        Vertex edge3 = transformedVertices[3 * i + 2];
        assembledVertices[i] = { edge1, edge2, edge3 };
    }
    std::vector<Fragment> fragments;
    for (size_t i = 0; i < assembledVertices.size(); ++i) {
        std::vector<Fragment> rasterizedTriangle = triangle(
                assembledVertices[i][0],
                assembledVertices[i][1],
                assembledVertices[i][2]
        );
        fragments.insert(fragments.end(), rasterizedTriangle.begin(), rasterizedTriangle.end());
    }
    for (size_t i = 0; i < fragments.size(); ++i) {
        Fragment& fragment = fragments[i];

        if (uniforms.objectType == ObjectType::SOL) {
            fragmentShaderSun(fragment);
        } else if (uniforms.objectType == ObjectType::MARS) {
            fragmentShaderMars(fragment);
        } else if (uniforms.objectType == ObjectType::EARTH) {
            fragmentShaderEarth(fragment);
        } else if (uniforms.objectType == ObjectType::VENUS) {
            fragmentShaderVenus(fragment);
        } else if (uniforms.objectType == ObjectType::SATURN) {
            fragmentShaderSaturn(fragment);
        }
        point(fragment);
    }
}

glm::mat4 createViewportMatrix(size_t screenWidth, size_t screenHeight) {
    glm::mat4 viewport = glm::mat4(1.0f);
    viewport = glm::scale(viewport, glm::vec3(screenWidth / 2.0f, screenHeight / 2.0f, 0.5f));
    viewport = glm::translate(viewport, glm::vec3(1.0f, 1.0f, 0.5f));

    return viewport;
}

struct Planet {
    ObjectType type;
    float Radius;
    float escala_F;
    float Velocidad__;
    float Angulo_P;
};


struct PlanetOrbit {
    glm::vec2 lastPoint;
    std::vector<glm::vec2> points;
};


void drawOrbit(const Planet& planet, const Uniforms& uniforms) {
    const int numSegments = 100;

    // Color de órbita
    Fragment frag;
    frag.color = Color(1.0f, 1.0f, 1.0f);

    glm::vec4 planetPosition = uniforms.projection * uniforms.view * glm::vec4(glm::vec3(uniforms.model[3]), 1.0f);
    glm::vec2 screenPos = glm::vec2((planetPosition.x / planetPosition.w + 1.0f) * 0.5f * SCREEN_WIDTH,
                                    (1.0f - planetPosition.y / planetPosition.w) * 0.5f * SCREEN_HEIGHT);

    static std::vector<glm::vec2> points;  // Lista de puntos en el borde

    // Almacenar solo el punto inicial en el borde
    if (points.size() == 0) {
        points.push_back(screenPos);
    }

    for (int i = 0; i <= numSegments; ++i) {
        float theta = planet.Angulo_P - static_cast<float>(i) / numSegments * glm::two_pi<float>();
        float x = screenPos.x + planet.Radius * cos(theta);
        float y = screenPos.y - planet.Radius * sin(theta);

        if (points.size() > 0) {
            points.push_back(glm::vec2(x, y));
        }
    }

    // Dibujar líneas entre puntos, excluyendo el primer punto
    for (int i = 0; i < points.size() - 1; i++) {
        glm::vec3 p1(points[i].x, points[i].y, 0);
        glm::vec3 p2(points[i + 1].x, points[i + 1].y, 0);

        std::vector<Fragment> fragments = line(p1, p2);

        // Dibujar fragments
        for (Fragment f : fragments) {
            // Asignar color blanco
            f.color = frag.color;
            point(f);
        }
    }

    if (points.size() > 1000) {
        points.erase(points.begin(), points.begin() + points.size() - 1000);
    }
}



std::vector<Planet> planets;
int currentPlanet = 0;

int main(int argc, char* argv[]) {
    if (!init()) {
        return 1;
    }
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec3> texCoords;
    std::vector<Face> faces;
    std::vector<glm::vec3> vertexBufferObject;

    loadOBJ("../models/sphere.obj", vertices, normals, texCoords, faces);

    for (const auto& face : faces)
    {
        for (int i = 0; i < 3; ++i)
        {
            glm::vec3 vertexPosition = vertices[face.vertexIndices[i]];
            glm::vec3 vertexNormal = normals[face.normalIndices[i]];
            glm::vec3 vertexTexture = texCoords[face.texIndices[i]];
            vertexBufferObject.push_back(vertexPosition);
            vertexBufferObject.push_back(vertexNormal);
            vertexBufferObject.push_back(vertexTexture);
        }
    }

    Uniforms uniforms;

    glm::mat4 view = glm::mat4(1);
    glm::mat4 projection = glm::mat4(1);
    glm::vec3 translationVector(0.0f, 0.0f, 0.0f);
    float a = 45.0f;
    glm::vec3 rotationAxis(0.0f, 1.0f, 0.0f);
    glm::vec3 escala_F(1.0f, 1.0f, 1.0f);

    Camera camera;
    camera.cameraPosition = glm::vec3(0.0f, 0.0f, 1.5f);
    camera.targetPosition = glm::vec3(0.0f, 0.0f, 0.0f);
    camera.upVector = glm::vec3(0.0f, 1.0f, 0.0f);
    float fovInDegrees = 45.0f;
    float aspectRatio = static_cast<float>(SCREEN_WIDTH) / static_cast<float>(SCREEN_HEIGHT);
    float nearClip = 0.1f;
    float farClip = 100.0f;
    uniforms.projection = glm::perspective(glm::radians(fovInDegrees), aspectRatio, nearClip, farClip);

    uniforms.viewport = createViewportMatrix(SCREEN_WIDTH, SCREEN_HEIGHT);
    Uint32 frameStart, frameTime;
    std::string title = "FPS: ";
    int speed = 10;
    planets.push_back({ ObjectType::SOL, 0.1f, 0.15f, 0.0f, 0.0f });
    planets.push_back({ ObjectType::EARTH, 0.25f, 0.06f,0.07f, 0.0f });
    planets.push_back({ ObjectType::SATURN, 0.35f, 0.06f,0.05f, 0.0f });
    planets.push_back({ ObjectType::MARS, 0.45f, 0.08f,0.03f, 0.0f });
    planets.push_back({ ObjectType::VENUS, 0.56f, 0.08f,0.01f, 0.0f });

    bool running = true;
    float fixedDeltaTime = 0.2f;
    while (running) {
        frame += 1;
        frameStart = SDL_GetTicks();

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }

            if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_SPACE:
                        currentPlanet = (currentPlanet + 1) % planets.size();
                        break;
                    case SDLK_LEFT:
                        camera.cameraPosition.x -= 0.1f;
                        break;
                    case SDLK_RIGHT:
                        camera.cameraPosition.x += 0.1f;
                        break;
                    case SDLK_UP:
                        camera.cameraPosition.y += 0.1f;
                        break;
                    case SDLK_DOWN:
                        camera.cameraPosition.y -= 0.1f;
                        break;
                    case SDLK_1:
                        camera.cameraPosition.z -= 0.1f;
                        break;
                    case SDLK_2:
                        camera.cameraPosition.z += 0.1f;
                        break;
                }
            }
        }

        uniforms.view = glm::lookAt(
                camera.cameraPosition,
                camera.targetPosition,
                glm::vec3(0.0f, 1.0f, 0.0f)  // Up vector changed to positive y-axis
        );

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        clearFramebuffer();
        int numStars = rand() % 500;
        for(int i = 0; i < numStars; i++) {

            int x = rand() % SCREEN_WIDTH;
            int y = rand() % SCREEN_HEIGHT;

            Fragment star;
            star.x = x;
            star.y = y;
            star.color = Color(1.0f, 1.0f, 1.0f);

            point(star);
        }


        for (auto& planet : planets) {
            uniforms.objectType = planet.type;
            glm::vec3 systemOffset(-0.1f, 0.0f, 0.0f);
            float Angulo_P = planet.Angulo_P;
            float Radius = planet.Radius;
            float orbitX = Radius * cos(Angulo_P);
            float orbitY = Radius * sin(Angulo_P);
            glm::mat4 translate = glm::translate(glm::mat4(1.0f), systemOffset);
            glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(planet.Angulo_P), glm::vec3(0.0f, 1.0f, 0.0f));  // Rotación en sentido horario alrededor del eje y
            glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(orbitX, 0.0f, orbitY));  // Cambié la posición para evitar el giro adicional en z
            glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(planet.escala_F));
            glm::mat4 model = translate * rotation * translation * scale;  // Cambié el orden para evitar un giro adicional

            uniforms.model = model;

            drawOrbit(planet, uniforms);

            render(vertexBufferObject, uniforms);
            planet.Angulo_P += planet.Velocidad__ * fixedDeltaTime;
        }

        renderBuffer(renderer);

        frameTime = SDL_GetTicks() - frameStart;
        if (frameTime > 0) {
            std::ostringstream titleStream;
            titleStream << "FPS: " << 1000.0 / frameTime;  // Milliseconds to seconds
            SDL_SetWindowTitle(window, titleStream.str().c_str());
        }
        SDL_Delay(16);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}