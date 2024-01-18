#pragma once

#include <vector>
#include <random>
#include <numeric>
#include <cmath>
#include "Vertex.h"
#include <iostream>
#include <list>
#include <limits>


/*PERLIN*/
extern const unsigned int PERLIN_SIZE;
extern float hScale;
extern float noiseScale;

void computeTerrainTangentBasis(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices);


inline void perlinNoiseInit(std::vector<int>& perlinG, int& seed) {
    perlinG.resize((size_t)PERLIN_SIZE * 2);
    std::iota(perlinG.begin(), perlinG.end(), 0);
    std::mt19937 g(seed);
    std::shuffle(perlinG.begin(), perlinG.begin() + PERLIN_SIZE, g);
    std::copy_n(perlinG.begin(), PERLIN_SIZE, perlinG.begin() + PERLIN_SIZE);
}

inline double perlinNoiseFade(double t) {
    return t * t * t * (t * (t * 6 - 15) + 10);
}

inline double perlinNoiseGradient(int hash, double x, double y) {
    hash &= 7;
    double u = hash < 4 ? x : y;
    double v = hash < 4 ? y : x;
    return ((hash & 1) ? -u : u) + ((hash & 2) ? -2.0 * v : 2.0 * v);
}

inline double perlinNoiseLerp(double t, double a, double b) {
    return a + t * (b - a);
}


inline double perlinNoise2D(double x, double y, std::vector<int>& perlinG) {
    
    int X = static_cast<int>(std::floor(x)) & (PERLIN_SIZE - 1);
    int Y = static_cast<int>(std::floor(y)) & (PERLIN_SIZE - 1);
    x -= std::floor(x);
    y -= std::floor(y);
    double u = perlinNoiseFade(x);
    double v = perlinNoiseFade(y);
    int A = perlinG[X] + Y, AA = perlinG[A], AB = perlinG[(size_t)A + 1],
        B = perlinG[(size_t)X + 1] + Y, BA = perlinG[B], BB = perlinG[(size_t)B + 1];

    return perlinNoiseLerp(v, perlinNoiseLerp(u, perlinNoiseGradient(perlinG[AA], x, y), perlinNoiseGradient(perlinG[BA], x - 1, y)),
        perlinNoiseLerp(u, perlinNoiseGradient(perlinG[AB], x, y - 1), perlinNoiseGradient(perlinG[BB], x - 1, y - 1)));
}

inline float fractalBrownianMotion(double x, double y, int octaves, float persistence, std::vector<int>& perlinG) {

    double total = 0;
    double frequency = 1;
    double amplitude = 1;
    double maxValue = 0;  // Used for normalizing result to 0.0 - 1.0
    for (int i = 0; i < octaves; i++) {
        total += perlinNoise2D(x * frequency, y * frequency, perlinG) * amplitude;

        maxValue += amplitude;

        amplitude *= persistence;
        frequency *= 2;
    }

    return static_cast<float>(total / maxValue);
}


inline void generateTerrainVerticesIndices(int& width, int& height, float& heightScale, std::vector<Vertex>& terrainVertices, std::vector<unsigned int>& terrainIndices, std::vector<int>& perlinG, float& outMinHeight, float& outMaxHeight) {

    std::vector<glm::vec3> faceNormals;
    float texRepeat = 4;
    // Generate vertices
    for (int z = 0; z < height; ++z) {
        for (int x = 0; x < width; ++x) {

            float localHeight = fractalBrownianMotion(x / (float)width, z / (float)height, 5, 0.5f, perlinG) * heightScale;
            Vertex vertex;
            vertex.vPos = glm::vec3(x - width/2, localHeight, z - height/2);
            vertex.vTexCoords = glm::vec2(x / (float)width, z / (float)height);
            vertex.vTexCoords.x = vertex.vTexCoords.x * texRepeat;
            vertex.vTexCoords.y = vertex.vTexCoords.y * texRepeat;
            vertex.vNormals = glm::vec3(0, 0, 0);
            vertex.vTangent = glm::vec3(0, 0, 0);
            vertex.vBiTangent = glm::vec3(0, 0, 0);

            terrainVertices.push_back(vertex);
        }
    }

    // Generate indices
    for (int z = 0; z < height - 1; ++z) {
        for (int x = 0; x < width - 1; ++x) {
            int topLeft = (z * width) + x;
            int topRight = topLeft + 1;
            int bottomLeft = ((z + 1) * width) + x;
            int bottomRight = bottomLeft + 1;

            terrainIndices.push_back(topLeft);
            terrainIndices.push_back(bottomRight);
            terrainIndices.push_back(bottomLeft);

            terrainIndices.push_back(topLeft);
            terrainIndices.push_back(topRight);
            terrainIndices.push_back(bottomRight);

            glm::vec3 edge1 = terrainVertices[topRight].vPos - terrainVertices[topLeft].vPos;
            glm::vec3 edge2 = terrainVertices[bottomRight].vPos - terrainVertices[topLeft].vPos;
            faceNormals.push_back(glm::normalize(glm::cross(edge2, edge1)));

            edge1 = terrainVertices[bottomRight].vPos - terrainVertices[topLeft].vPos;
            edge2 = terrainVertices[bottomLeft].vPos - terrainVertices[topLeft].vPos;
            faceNormals.push_back(glm::normalize(glm::cross(edge2, edge1)));

        }
    }

    // Compute vertex normals by averaging face normals
    for (unsigned int i = 0; i < terrainIndices.size(); i += 3) {
        for (unsigned int j = 0; j < 3; ++j) {
            terrainVertices[terrainIndices[i + j]].vNormals += faceNormals[i / 3];
        }
    }
    for (auto& vertex : terrainVertices) {
        vertex.vNormals = glm::normalize(vertex.vNormals);
        vertex.vNormals.y = abs(vertex.vNormals.y);
    }

    outMinHeight = 0;
    outMaxHeight = 0;

    for (auto& vertex : terrainVertices) {
        if (vertex.vPos.y < outMinHeight) {
            outMinHeight = vertex.vPos.y;
        }
        if (vertex.vPos.y > outMaxHeight) {
            outMaxHeight = vertex.vPos.y;
        }
    }


    computeTerrainTangentBasis(terrainVertices, terrainIndices);
}

inline void computeTerrainTangentBasis(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices)
{
    std::vector<glm::vec3> tangents(vertices.size(), glm::vec3(0.0f));
    std::vector<glm::vec3> bitangents(vertices.size(), glm::vec3(0.0f));

    for (size_t i = 0; i < indices.size(); i += 3) {
        Vertex& v0 = vertices[indices[i]];
        Vertex& v1 = vertices[indices[i + 1]];
        Vertex& v2 = vertices[indices[i + 2]];

        /*Calculate the tangent and bitangent as before*/
        glm::vec3 edge1 = v1.vPos - v0.vPos;
        glm::vec3 edge2 = v2.vPos - v0.vPos;
        glm::vec2 deltaUV1 = v1.vTexCoords - v0.vTexCoords;
        glm::vec2 deltaUV2 = v2.vTexCoords - v0.vTexCoords;

        float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

        glm::vec3 tangent;
        tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

        glm::vec3 bitangent;
        bitangent.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
        bitangent.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
        bitangent.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);

        /*Accumulate the tangent and bitangent for each vertex of the triangle*/
        tangents[indices[i]] += tangent;
        tangents[indices[i + 1]] += tangent;
        tangents[indices[i + 2]] += tangent;

        bitangents[indices[i]] += bitangent;
        bitangents[indices[i + 1]] += bitangent;
        bitangents[indices[i + 2]] += bitangent;
    }

    /*Normalize and orthogonalize tangent and bitangent*/
    for (unsigned int i = 0; i < vertices.size(); ++i) {
        Vertex& vertex = vertices[i];

        if (glm::length(tangents[i]) > 0.0f)
            vertex.vTangent = glm::normalize(tangents[i]);
        else
            vertex.vTangent = glm::vec3(0.0f);

        if (glm::length(bitangents[i]) > 0.0f)
            vertex.vBiTangent = glm::normalize(bitangents[i]);
        else
            vertex.vBiTangent = glm::vec3(0.0f);

        /*Orthogonalize*/
        vertex.vTangent = glm::normalize(vertex.vTangent - vertex.vNormals * glm::dot(vertex.vNormals, vertex.vTangent));
        vertex.vBiTangent = glm::cross(vertex.vNormals, vertex.vTangent);
    }
}

inline std::vector<float> generateBlendMap(const std::vector<Vertex>& terrainVertices, int width, int height, float minHeight, float maxHeight) {
    std::vector<float> blendMap(width * height);

    for (int z = 0; z < height; ++z) {
        for (int x = 0; x < width; ++x) {
            /*Get the vertex for this point on the terrain*/
            const Vertex& vertex = terrainVertices[(size_t)z * width + x];

            /*Normalize the height to a value between 0 and 1. 0 represents grass, 1 represents rock */
            float normalizedHeight = (vertex.vPos.y - minHeight) / (maxHeight - minHeight);

            /*Clamp the value between 0 and 1*/
            normalizedHeight = std::max(0.f, std::min(1.f, normalizedHeight));

            /*Store the normalized height in the blendmap*/
            blendMap[(size_t)z * width + x] = normalizedHeight;
        }
    }

    return blendMap;
}
