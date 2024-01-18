#pragma once
#include <glm/glm.hpp>

struct Vertex
{
    glm::vec3 vPos{ 0.f };
    glm::vec2 vTexCoords{ 0.f };
    glm::vec3 vNormals{ 0.f };
    glm::vec3 vTangent{ 0.f };
    glm::vec3 vBiTangent{ 0.f };

    Vertex(glm::vec3 vP, glm::vec3 vTC, glm::vec3 vN, glm::vec3 vT, glm::vec3 vBT) : vPos(vP), vTexCoords(vTC), vNormals(vN), vTangent(vT), vBiTangent(vBT) {};

    Vertex() {}
};