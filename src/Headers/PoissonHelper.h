#pragma once

#include <iostream>
#include <glad/glad.h>
#include <GLFWLib/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <random>
#include <unordered_map>
#include <queue>
#include <vector>

// The Grid structure to store points.
struct Grid {
    std::unordered_map<int, std::unordered_map<int, std::unordered_map<int, glm::vec3>>> data;
};

// Place point into grid
inline void placePoint(Grid& grid, glm::vec3 point, float cellSize) {
    int gridX = static_cast<int>(floor(point.x / cellSize));
    int gridY = static_cast<int>(floor(point.y / cellSize));
    int gridZ = static_cast<int>(floor(point.z / cellSize));
    grid.data[gridX][gridY][gridZ] = point;
}

// Check if there's a point in neighbourhood
inline bool inNeighbourhood(Grid& grid, glm::vec3 point, float minDist, float cellSize) {
    int gridX = static_cast<int>(floor(point.x / cellSize));
    int gridY = static_cast<int>(floor(point.y / cellSize));
    int gridZ = static_cast<int>(floor(point.z / cellSize));

    for (int x = gridX - 2; x <= gridX + 2; ++x)
        for (int y = gridY - 2; y <= gridY + 2; ++y)
            for (int z = gridZ - 2; z <= gridZ + 2; ++z) {
                if (grid.data[x][y][z] != glm::vec3(0, 0, 0) && glm::distance(grid.data[x][y][z], point) < minDist)
                    return true;
            }
    return false;
}

// Generate a point in annulus
inline glm::vec3 generateRandomPointAround(glm::vec3 point, float minDist)
{
    float M_PI = 3.141592653589f;
    float r1 = static_cast <float> (rand()) / static_cast <float> (RAND_MAX); //random point between 0 and 1
    float r2 = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
    float r3 = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
    //random radius between mindist and 2 * mindist
    float radius = minDist * (r1 + 1);
    //random angle
    float angle1 = 2 * M_PI * r2;
    float angle2 = 2 * M_PI * r3;
    //the new point is generated around the point (x, y, z)
    float newX = point.x + radius * cos(angle1) * sin(angle2);
    float newY = point.y + radius * sin(angle1) * sin(angle2);
    float newZ = point.z + radius * cos(angle2);
    return glm::vec3(newX, newY, newZ);
}

// Poisson disk sampling
inline std::vector<glm::vec3> poissonDiskSampling3D(float width, float height, float depth, float minDist, int numStones) {
    float cellSize = float(minDist / sqrt(3));
    Grid grid;
    std::queue<glm::vec3> processList;
    std::vector<glm::vec3> samplePoints;

    glm::vec3 firstPoint(0.0f, 0.0f, 0.0f); // World origin
    processList.push(firstPoint);
    samplePoints.push_back(firstPoint);
    placePoint(grid, firstPoint, cellSize);

    //generate other points from points in queue.
    while (!processList.empty() && samplePoints.size() < numStones) {
        glm::vec3 point = processList.front();
        processList.pop();

        for (int i = 0; i < numStones && samplePoints.size() < numStones; ++i) {
            glm::vec3 newPoint = generateRandomPointAround(point, minDist);
            if (newPoint.x >= -width / 2 && newPoint.y >= -height / 2 && newPoint.z >= -depth / 2 &&
                newPoint.x <= width / 2 && newPoint.y <= height / 2 && newPoint.z <= depth / 2 &&
                !inNeighbourhood(grid, newPoint, minDist, cellSize)) {
                processList.push(newPoint);
                samplePoints.push_back(newPoint);
                placePoint(grid, newPoint, cellSize);
            }
        }
    }
    return samplePoints;
}

inline double getRandomDouble()
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<> dis(0.0, 1.0);
    return dis(gen);
}

