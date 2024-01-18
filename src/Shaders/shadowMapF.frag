

#version 330 core
out vec4 FragColor;

in vec2 outTexCoords;

uniform sampler2D depthMap;
uniform float nearPlane;
uniform float farPlane;

void main()
{             
    float depthValue = texture(depthMap, outTexCoords).r;  // Read the depth value from the texture
    // Convert the depth to a linear range
    float linearDepth = nearPlane / (farPlane - depthValue * (farPlane - nearPlane));
    FragColor = vec4(vec3(linearDepth), 1.0);
}

