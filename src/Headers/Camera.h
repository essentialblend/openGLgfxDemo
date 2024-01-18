#pragma once

#include "../includes/GLAD/glad.h"
#include "../includes/glm/glm.hpp"
#include "../includes/glm/gtc/matrix_transform.hpp"

#include <vector>

enum cameraMovement
{
	FORWARD, BACKWARD, LEFT, RIGHT
};

/*Defaults*/
const float defaultYawValue = -90.f;
const float defaultPitchValue = 0.f;
const float defaultSpeedValue = 7.5;
const float defaultSensValue = 0.1f;
const float defaultZoomValue = 45.f;

class Camera
{
public:
	glm::vec3 Position;
	glm::vec3 Front;
	glm::vec3 Up;
	glm::vec3 Right;
	glm::vec3 WorldUp;

	float eulerYaw{ 0 };
	float eulerPitch{ 0 };

	float movementSpeed{ 0 };
	float mouseSens{ 0 };
	float zoom{ 0 };
	
	/*Vector Constructor*/
	Camera(glm::vec3 position = glm::vec3(0.f), glm::vec3 up = glm::vec3(0.f, 1.f, 0.f), float yaw = defaultYawValue, float pitch = defaultPitchValue) : Front(glm::vec3(0.f, 0.f, -1.f)), movementSpeed(defaultSpeedValue), mouseSens(defaultSensValue), zoom(defaultZoomValue)
	{
		Position = position;
		WorldUp = up;
		eulerYaw = yaw;
		eulerPitch = pitch;
		updateCameraVectors();
	}

	/*Scalar Constructor*/
	Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch) : Front(glm::vec3(0.0f, 0.0f, -1.0f)), movementSpeed(defaultSpeedValue), mouseSens(defaultSensValue), zoom(defaultZoomValue)
	{
		Position = glm::vec3(posX, posY, posZ);
		WorldUp = glm::vec3(upX, upY, upZ);
		eulerYaw = yaw;
		eulerPitch = pitch;
		updateCameraVectors();
	}

	/*Return the view matrix calculated with LookAt and Euler*/
	glm::mat4 getViewMatrix()
	{
		return glm::lookAt(Position, Position + Front, Up);
	}

	/*Process keyboard input*/
	void processKeyboardInput(cameraMovement direction, float deltaTime)
	{
		float velocity = movementSpeed * deltaTime;
		glm::vec3 newPos = Position;

		glm::vec3 planeForward = glm::normalize(glm::vec3(Front.x, 0, Front.z));

		if (direction == FORWARD)
			newPos += planeForward * velocity;
		if (direction == BACKWARD)
			newPos -= planeForward * velocity;
		if (direction == LEFT)
			newPos -= Right * velocity;
		if (direction == RIGHT)
			newPos += Right * velocity;

		// Check terrain collision
		float terrainHeight = getHeightAtPosition(blendMap, 50, 50, newPos);
		if (newPos.y < terrainHeight)
			newPos.y = terrainHeight + 1; // add some offset to avoid sinking into the terrain



		Position = newPos;
	}

	/*Process mouse input*/
	void processMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true)
	{
		xoffset *= mouseSens;
		yoffset *= mouseSens;

		eulerYaw += xoffset;
		eulerPitch += yoffset;

		if (constrainPitch)
		{
			if (eulerPitch > 89.0f)
				eulerPitch = 89.0f;
			if (eulerPitch < -89.0f)
				eulerPitch = -89.0f;
		}
		updateCameraVectors();

	}

	float getHeightAtPosition(const std::vector<float>& blendMap, int width, int height, glm::vec3 position)
	{
		/*Normalize the position to the range[0, width] and [0, height] for x and z respectively*/
		float normalizedX = position.x / width;
		float normalizedZ = position.z / height;

		// Find the four corners of the grid cell the position is in
		int x1 = std::floor(normalizedX);
		int x2 = x1 + 1;
		int z1 = std::floor(normalizedZ);
		int z2 = z1 + 1;

		/*Ensure the position is within the grid*/
		if (x1 < 0 || x2 >= width || z1 < 0 || z2 >= height)
		{
			return 0.0f;
		}

		/*Find the height at each corner of the grid cell*/
		float h1 = blendMap[x1 + z1 * width];
		float h2 = blendMap[x2 + z1 * width];
		float h3 = blendMap[x1 + z2 * width];
		float h4 = blendMap[x2 + z2 * width];

		// Calculate the x and z fractions from the position within the grid cell
		float fracX = normalizedX - x1;
		float fracZ = normalizedZ - z1;

		/*Perform bilinear interpolation to find the height at the exact position.*/
		float fheight = h1 * (1 - fracX) * (1 - fracZ) +
			h2 * fracX * (1 - fracZ) +
			h3 * (1 - fracX) * fracZ +
			h4 * fracX * fracZ;

		return fheight;
	}

	void setBlendMap(std::vector<float> inBlendMap)
	{
		blendMap = inBlendMap;
	}

private:
	std::vector<float> blendMap;
	void updateCameraVectors()
	{
		glm::vec3 front;
		front.x = cos(glm::radians(eulerYaw)) * cos(glm::radians(eulerPitch));
		front.y = sin(glm::radians(eulerPitch));
		front.z = sin(glm::radians(eulerYaw)) * cos(glm::radians(eulerPitch));
		
		Front = glm::normalize(front);
		Right = glm::normalize(glm::cross(Front, WorldUp));
		Up = glm::normalize(glm::cross(Right, Front));
	}

};