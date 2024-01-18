#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBiTangent;
layout (location = 5) in mat4 aInstanceMatrix;

out VS_OUT
{
	vec3 outNormal;
	vec3 outFragPos;
	vec2 outTexCoords;
	vec4 outFragPosLightSpace;
	vec3 outTangentLightDir;
	vec3 outTangentViewPos;
	vec3 outTangent;
	vec3 outBiTangent;
	vec4 outDebugTangentColor;

} vertOuts;

uniform mat4 modelMat;
uniform mat4 viewMat;
uniform mat4 projMat;
uniform vec3 viewPos;
uniform mat4 lightSpaceMatrix;
uniform vec3 vertexLightDirection;
uniform bool isInstanced;

uniform float uTime;
uniform float noiseScale;
uniform float heightScale;

float SimpleNoise(vec2 p);
vec2 SimpleNoiseGradient(vec2 p);


void main()
{

	vec3 modPos = aPos;
	float noise = SimpleNoise(aPos.xz * noiseScale + vec2(uTime));
	modPos = aPos + vec3(0.f, noise * heightScale, 0.f);

	mat4 finalModelMat = isInstanced ? aInstanceMatrix : modelMat;
    vertOuts.outFragPos = vec3(finalModelMat * vec4(aPos, 1.0));
    vertOuts.outNormal = mat3(transpose(inverse(finalModelMat))) * aNormal;
	vertOuts.outTexCoords = aTexCoords;
	vertOuts.outFragPosLightSpace = lightSpaceMatrix * vec4(vertOuts.outFragPos, 1.f);
	vec3 normVertexLightDirection = normalize(vertexLightDirection);

	/*NORMAL MAPPING*/
	vertOuts.outTangent = aTangent;
	vertOuts.outBiTangent = aBiTangent;
	mat3 normalMatrix = transpose(inverse(mat3(finalModelMat)));
	vec3 T = normalize(normalMatrix * aTangent);
	vec3 N = normalize(normalMatrix * aNormal);
	T = normalize(T - dot(T, N) * N);
	vec3 B = cross(N, T);

	mat3 TBNMatrix = transpose(mat3(T, B, N));
	vertOuts.outTangentLightDir = TBNMatrix * (normVertexLightDirection);
    vertOuts.outTangentViewPos = TBNMatrix * (viewPos - vertOuts.outFragPos);

    gl_Position = projMat * viewMat * finalModelMat * vec4(aPos, 1.f);
}

float SimpleNoise(vec2 p)
{
    return sin(p.x * 10.0) * cos(p.y * 10.0);
}

vec2 SimpleNoiseGradient(vec2 p)
{
    const float eps = 0.001; // a small number
    vec2 e = vec2(eps, 0.0);
    return vec2(
        SimpleNoise(p + e) - SimpleNoise(p - e),
        SimpleNoise(p + vec2(0.0, eps)) - SimpleNoise(p - vec2(0.0, eps))
    ) / (2.0 * eps);
}