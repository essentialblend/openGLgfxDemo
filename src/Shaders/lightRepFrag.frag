#version 330 core
out vec4 FragColor;
in VS_OUT {
    vec3 outNormal;
	vec3 outFragPos;
	vec2 outTexCoords;
    //vec4 outFragPosLightSpace;
    //vec3 outTangentLightPos;
    //vec3 outTangentViewPos;
    //vec3 outTangentFragPos;
} fragIns;

//uniform vec3 viewPos;
//uniform Material currentMaterial;
//uniform DirectionalLight dirLight;

void main()
{
    FragColor = vec4(vec3(1.f), 1.0);
} 