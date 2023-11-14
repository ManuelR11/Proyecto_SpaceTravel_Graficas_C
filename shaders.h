#pragma once
#include "glm/geometric.hpp"
#include "glm/glm.hpp"
#include "FastNoise.h"
#include "uniforms.h"
#include "fragment.h"
#include "noise.h"
#include "print.h"

static int frame = 0;

Vertex vertexShader(const Vertex& vertex, const Uniforms& uniforms) {
    // Apply transformations to the input vertex using the matrices from the uniforms
    glm::vec4 clipSpaceVertex = uniforms.projection * uniforms.view * uniforms.model * glm::vec4(vertex.position, 1.0f);

    // Perspective divide
    glm::vec3 ndcVertex = glm::vec3(clipSpaceVertex) / clipSpaceVertex.w;

    // Apply the viewport transform
    glm::vec4 screenVertex = uniforms.viewport * glm::vec4(ndcVertex, 1.0f);

    // Transform the normal
    glm::vec3 transformedNormal = glm::mat3(uniforms.model) * vertex.normal;
    transformedNormal = glm::normalize(transformedNormal);

    glm::vec3 transformedWorldPosition = glm::vec3(uniforms.model * glm::vec4(vertex.position, 1.0f));

    // Return the transformed vertex as a vec3
    return Vertex{
            glm::vec3(screenVertex),
            transformedNormal,
            vertex.tex,
            transformedWorldPosition,
            vertex.position
    };
}

Fragment fragmentShaderStripes(Fragment& fragment) {
    // Define the base color for Mars

    // Create some noise based on the fragment position
    /* float noise = glm::fract(sin(glm::dot(glm::vec2(fragment.x, fragment.y), glm::vec2(12.9898f, 78.233f))) * 43758.5453f); */

    /* float noise = glm::fract(sin(glm::dot(fragment.worldPos, glm::vec3(12.9898f, 78.233f, 56.789f))) * 43758.5453f); */
    // Slightly adjust the color based on the noise
    /* Color color = marsColor + Color(static_cast<int>(noise * 25), static_cast<int>(noise * 10), static_cast<int>(noise * 5)); */
    /* glm::vec3 wc = fragment.worldPos * 1.0f + 0.5f; */
    /* Color color = Color(wc.r, wc.g, wc.b); */

    // Define a color for Jupiter. You may want to adjust this to better match Jupiter's appearance.
    Color baseColor = Color(125, 67, 37);  // This color is a kind of dark orange

    // Create stripes based on the fragment's Y position in world space. The frequency of the stripes
    // can be controlled by scaling the Y coordinate.
    // The number 10.0f determines the frequency of the stripes, adjust it to get the desired effect.
    float stripePattern = glm::abs(glm::cos(fragment.originalPos.y * 20.0f));

    // Interpolate between the base color and a darker version of the base color based on the stripe pattern.
    // This will create dark stripes on the sphere.
    Color stripeColor = baseColor * (0.5f + 0.5f * stripePattern);

    // Apply lighting intensity
    stripeColor = stripeColor * fragment.intensity;

    // Set the fragment color
    Color color = stripeColor;



    // Define the direction to the center of the circle in world space
    // Apply lighting intensity
    color = color * fragment.intensity;

    // Set the fragment color
    fragment.color = color;

    return fragment;
}





Fragment fragmentShaderEarth(Fragment& fragment) {
    Color color;

    glm::vec3 groundColor = glm::vec3(0, 0.5, 0);
    glm::vec3 oceanColor = glm::vec3(0, 0, 1);
    glm::vec3 cloudColor = glm::vec3(1.0f, 1.0f, 1.0f);

    float x = fragment.originalPos.x;
    float y = fragment.originalPos.y;
    float z = fragment.originalPos.z;
    /* glm::vec2 uv = glm::vec2(fragment.originalPos.x, fragment.originalPos.y); */
    float radius = sqrt(x*x + y*y + z*z);

    /* glm::vec2 uv = glm::vec2( */
    /*     atan2(x, z), */
    /*     acos(y/sqrt(x*x + y*y + z*z)) */
    /* ); */

    glm::vec3 uv = glm::vec3(
            atan2(x, z),
            acos(y / radius),
            radius
    );

    glm::vec3 uv2 = glm::vec3(
            atan2(x + 10, z),
            acos(y / radius),
            radius
    );


    FastNoiseLite noiseGenerator;
    noiseGenerator.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    /* noiseGenerator.SetRotationType3D(FastNoiseLite::RotationType3D_ImproveXYPlanes); */
    /* noiseGenerator.DomainWarp(uv.x, uv.y, uv.z); */

    float ox = 1200.0f;
    float oy = 3000.0f;
    float zoom = 100.0f;

    /* float noiseValue1 = noiseGenerator.GetNoise(uv.x * zoom, uv.y * zoom); */
    /* float noiseValue2 = noiseGenerator.GetNoise(uv.y * zoom + 1000.0f, uv.x * zoom + 1000.0f); */
    /* float noiseValue = (noiseValue1 + noiseValue2) * 0.5f; */

    float noiseValue1 = noiseGenerator.GetNoise(uv.x * zoom, uv.y * zoom, uv.z * zoom);
    float noiseValue2 = noiseGenerator.GetNoise(uv2.x * zoom + ox, uv2.y * zoom, uv2.z * zoom + oy);
    float noiseValue = (noiseValue1 + noiseValue2) * 0.5f;



    glm::vec3 tmpColor = (noiseValue < 0.2f) ? oceanColor : groundColor;

    float oxc = 5500.0f;
    float oyc = 6900.0f;
    float zoomc = 300.0f;

    float noiseValueC = noiseGenerator.GetNoise((uv.x + oxc) * zoomc, (uv.y + oyc) * zoomc);

    if (noiseValueC > 0.5f) {
         tmpColor = cloudColor;
    }


    color = Color(tmpColor.x, tmpColor.y, tmpColor.z);



    fragment.color = color * fragment.intensity;

    return fragment;
}



// Fragment Shader
Fragment fragmentShaderVenus(Fragment& fragment) {

    Color color;

    // Coordenadas UV del fragmento
    glm::vec2 uv = glm::vec2(fragment.originalPos.x, fragment.originalPos.y);

    // Radio del planeta (Venus es un poco más pequeño que la Tierra)
    float planetRadius = 0.4;

    // Distancia al centro
    float distance = length(uv);

    // Color base amarillo claro (similar a Venus)
    Color planetColor(255, 230, 153); // Amarillo claro en la estructura Color

    // Ruido para variaciones de color (puedes ajustar los valores según tus preferencias)
    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);

    float noiseX = 1234.0;
    float noiseY = 5678.0;
    float noiseScale = 5000.0; // Aumenta la escala del ruido para hacer las manchas más grandes
    float threshold = 0.4; // Disminuye el umbral para hacer las manchas más grandes

    float noiseValue = noise.GetNoise((uv.x + noiseX) * noiseScale, (uv.y + noiseY) * noiseScale);

    // Agregar manchas de color naranja oscuro
    Color darkOrange(204, 85, 0); // Color naranja oscuro en la estructura Color

    if (noiseValue > threshold) {
        // Si el valor de ruido es mayor que el umbral, aplica el color naranja oscuro
        color = darkOrange;
    } else {
        // Si el valor de ruido es menor o igual que el umbral, utiliza el color base
        color = planetColor;
    }

    // Convertir a Color y aplicar intensidad
    color = color * fragment.intensity;

    fragment.color = color;

    return fragment;
}


Fragment fragmentShaderMars(Fragment& fragment) {
    Color color;

    // Coordenadas UV del fragmento
    glm::vec2 uv = glm::vec2(fragment.originalPos.x, fragment.originalPos.y);

    // Radio del planeta (Marte es más pequeño que la Tierra)
    float planetRadius = 0.4;

    // Distancia al centro
    float distance = length(uv);

    // Color base rojo (similar a Marte)
    Color planetColor(210, 0, 0); // Rojo en la estructura Color

    // Ruido para variaciones de color (puedes ajustar los valores según tus preferencias)
    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);

    float noiseX = 1234.0;
    float noiseY = 5678.0;
    float noiseScale = 5000.0; // Aumenta la escala del ruido para hacer los cráteres más grandes
    float threshold = 0.4; // Disminuye el umbral para hacer los cráteres más grandes

    float noiseValue = noise.GetNoise((uv.x + noiseX) * noiseScale, (uv.y + noiseY) * noiseScale);

    // Agregar cráteres que cubran todo el planeta
    float craterThreshold = 0.05; // Umbral para la generación de cráteres
    if (noiseValue < craterThreshold) {
        // Si el valor de ruido es menor que el umbral, crea un cráter
        Color craterColor(0, 0, 0); // Color rojizo
        color = craterColor;
    } else {
        // Si no estamos en un cráter, utiliza el color base
        color = planetColor;
    }

    // Convertir a Color y aplicar intensidad
    color = color * fragment.intensity;

    fragment.color = color;

    return fragment;
}




Fragment fragmentShaderSaturn(Fragment fragment) {

    Color color;

    // Coordenadas UV
    glm::vec2 uv = glm::vec2(fragment.originalPos.y, fragment.originalPos.x);

    // Radio del planeta
    float planetRadius = 0.95;

    // Distancia al centro
    float distance = length(uv);

    // Color base
    Color planetColor(201, 159, 79);

    // Primer ruido para nubes amarillas
    FastNoiseLite noise1;
    noise1.SetNoiseType(FastNoiseLite::NoiseType_Perlin);

    float noiseX = 12345.0;
    float noiseY = 67890.0;
    float noiseScale1 = 50000.0;

    float noiseValue1 = noise1.GetNoise((uv.x + noiseX) * noiseScale1,
                                        (uv.y + noiseY) * noiseScale1);

    float cloudThreshold1 = 0.1;

    // Segundo ruido para nubes moradas
    FastNoiseLite noise2;
    noise2.SetNoiseType(FastNoiseLite::NoiseType_Perlin);

    float noiseScale2 = 80000.0;

    float noiseValue2 = noise2.GetNoise((uv.x + noiseX) * noiseScale2,
                                        (uv.y + noiseY) * noiseScale2);

    float cloudThreshold2 = 0.08;

    // Combinar nubes
    if (noiseValue2 > cloudThreshold2) {
        // Nubes moradas
        color = Color(203, 123, 0);
    } else if (noiseValue1 > cloudThreshold1) {
        // Nubes amarillas
        color = Color(117, 49, 0);
    } else {
        // Superficie
        color = planetColor;
    }

    // Aplicar intensidad
    color = color * fragment.intensity;

    fragment.color = color;

    return fragment;

}





Fragment fragmentShaderSun(Fragment& fragment) {
    Color color;

    // Get UV coordinates
    glm::vec2 uv = glm::vec2(fragment.originalPos.x, fragment.originalPos.y);

    FastNoiseLite noiseGenerator;
    noiseGenerator.SetNoiseType(FastNoiseLite::NoiseType_Perlin);

    float offsetX = 10000.0f;
    float offsetY = 10000.0f;
    float scale = 7000.0f; // Ajusta la escala para cambiar el patrón de cráteres

    // Generate Perlin noise for craters
    float noiseValue = noiseGenerator.GetNoise((uv.x + offsetX) * scale, (uv.y + offsetY) * scale);

    // Map noise range to crater color (naranja)
    Color craterColor(1.0f, 0.5f, 0.0f); // Naranja
    float craterThreshold = 0.3; // Ajusta el umbral para cambiar el patrón de cráteres
    if (noiseValue > craterThreshold) {
        color = craterColor;
    } else {
        // Set sun color to rojizo
        glm::vec3 sunColor = glm::vec3(1.0f, 0.2f, 0.0f); // Rojo
        color = Color(sunColor.r, sunColor.g, sunColor.b);
    }

    fragment.color = color * fragment.intensity;

    return fragment;
}





Fragment fragmentShader(Fragment& fragment) {

    Color color;

    glm::vec3 brightColor = glm::vec3(1.0f, 0.6f, 0.0f);
    glm::vec3 darkColor = glm::vec3(0.8f, 0.2f, 0.0f);

    float x = fragment.originalPos.x;
    float y = fragment.originalPos.y;
    float z = fragment.originalPos.z;
    float radius = sqrt(x*x + y*y + z*z);

    glm::vec3 uv = glm::vec3(
            atan2(x, z),
            acos(y/radius),
            radius
    );

    FastNoiseLite noiseGenerator;


    noiseGenerator.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
    noiseGenerator.SetFrequency(0.02 + (10 - abs((static_cast<int>(frame/10.0f) % (2 * 10)) - 10))/2000.0f);

    float zoom = 1000.0f;


    float lat = uv.y;  // Latitude
    float lon = uv.x;  // Longitude

    // Normalize the direction vector
    glm::vec3 dir = glm::normalize(glm::vec3(sin(lat) * cos(lon), sin(lat) * sin(lon), cos(lat)));

    float noiseValue1 = noiseGenerator.GetNoise(uv.x * zoom, uv.y * zoom, uv.z * zoom);
    float noiseValue2 = noiseGenerator.GetNoise(uv.x * zoom + 1000.0f, uv.y * zoom + 1000.0f);
    float noiseValue = (noiseValue1 + noiseValue2) * 0.5f;

    float edgeFactorY = sin(dir.y);  // Value is 0 at the poles and 1 at the equator
    float edgeFactorX = sin(dir.x);  // Value is 0 at leftmost and rightmost points, 1 at front and back
    float edgeFactor = edgeFactorX * edgeFactorY;
    float alpha = 1.0f - glm::mix(1.0f, 0.0f, edgeFactor);  // Alpha is 1 at the poles and 0 at the equator

    glm::vec3 tmpColor = mix(brightColor, darkColor, noiseValue);

    color = Color(tmpColor.x, tmpColor.y, tmpColor.z, alpha);


    fragment.color = color * fragment.intensity;

    return fragment;
}
