#version 330 core
out vec4 FragColor;

in vec3 outTexCoords;

uniform samplerCube nightSkyboxCubemap;
uniform samplerCube daySkyboxCubemap;

uniform float interpFactor;

void main()
{    
	vec3 dayColor = texture(daySkyboxCubemap, outTexCoords).rgb;
	vec3 nightColor = texture(nightSkyboxCubemap, outTexCoords).rgb;
	vec3 finalColor = mix(nightColor, dayColor, interpFactor);
	FragColor = vec4(finalColor, 1.0);
}