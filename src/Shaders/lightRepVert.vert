#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;
layout (location = 2) in vec3 aNormal;

out VS_OUT
{
	vec3 outNormal;
	vec3 outFragPos;
	vec2 outTexCoords;
	//vec4 outFragPosLightSpace;
	//vec3 outTangentLightPos;
	//vec3 outTangentViewPos;
	//vec3 outTangentFragPos;

} vertOuts;

uniform mat4 modelMat;
uniform mat4 viewMat;
uniform mat4 projMat;

void main()
{
    vertOuts.outFragPos = vec3(modelMat * vec4(aPos, 1.0));
    vertOuts.outNormal = mat3(transpose(inverse(modelMat))) * aNormal;
	vertOuts.outTexCoords = aTexCoords;

    gl_Position = projMat * viewMat * vec4(vertOuts.outFragPos, 1.0);
}