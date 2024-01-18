#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "../includes/glm/glm.hpp"
#include <map>
#include "Vertex.h"



void computeTangentBasis(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices);

/*-----------PARSE OBJ----------*/
inline bool parseOBJFile(const std::string& filePath,
    std::vector<Vertex>& outVertices,
    std::vector<unsigned int>& orderedIndices,
    std::map<std::tuple<GLuint, GLuint, GLuint>, GLuint>& uniqueVertices
)
{
    std::ifstream OBJfile(filePath);
    if (!OBJfile.is_open()) {
        std::cout << "Unable to open file: " << filePath << std::endl;
        return false;
    }

    std::vector<Vertex> vertices;
    std::string currentLine;
    std::vector<glm::vec3> tempPos;
    std::vector<glm::vec2> tempTexC;
    std::vector<glm::vec3> tempNormals;

    while (std::getline(OBJfile, currentLine)) {
        std::istringstream lineStream(currentLine);
        std::string lineHeader;
        lineStream >> lineHeader;

        if (lineHeader == "v") {
            glm::vec3 pos{ 0.f };
            lineStream >> pos.x >> pos.y >> pos.z;
            tempPos.push_back(pos);

        }
        else if (lineHeader == "vt") {
            glm::vec2 tc{ 0.f };
            lineStream >> tc.x >> tc.y;
            tc.y = 1.f - tc.y;
            tempTexC.push_back(tc);

        }
        else if (lineHeader == "vn") {
            glm::vec3 normals{ 0.f };
            lineStream >> normals.x >> normals.y >> normals.z;
            tempNormals.push_back(normals);

        }
        else if (lineHeader == "f") {
            for (int i = 0; i < 3; ++i) {
                Vertex vertex;
                GLuint positionIndex, texcoordIndex, normalIndex;
                char slash;
                lineStream >> positionIndex >> slash >> texcoordIndex >> slash >> normalIndex;

                positionIndex--; 
                texcoordIndex--;
                normalIndex--;


                std::tuple<GLuint, GLuint, GLuint> uniqueKey = { positionIndex, texcoordIndex, normalIndex };
                auto it = uniqueVertices.find(uniqueKey);
                unsigned int index;
                if (it != uniqueVertices.end()) {
                    index = it->second;
                }
                else {
                    vertex.vPos = tempPos[positionIndex];
                    vertex.vTexCoords = tempTexC[texcoordIndex];
                    vertex.vNormals = tempNormals[normalIndex];
                    vertices.push_back(vertex);
                    index = static_cast<unsigned int>(vertices.size() - (1));
                    uniqueVertices[uniqueKey] = size_t(index);
                }
                orderedIndices.push_back(index);
            }
        }
    }
    OBJfile.close();

    outVertices = vertices;

    computeTangentBasis(outVertices, orderedIndices);

    return true;
}


inline void computeTangentBasis(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices)
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
