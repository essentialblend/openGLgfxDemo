#version 330 core
out vec4 FragColor;

struct Terrain
{
    sampler2D diffuseMap1;
    sampler2D specularMap1;
    sampler2D normalMap1;
    sampler2D AOMap1;
    sampler2D roughnessMap1;
    float shininess;
    
    sampler2D diffuseMap2;
    sampler2D specularMap2;
    sampler2D normalMap2;
    sampler2D AOMap2;
    sampler2D roughnessMap2;

    sampler2D blendMap;

    bool hasSpecularMap;

    vec3 tSpecularValues;
    
    
};


struct DirectionalLight 
{
    vec3 lightDirection;
    /*Redundant, but used for making debug easier*/
    vec3 lightPosition;

    vec3 ambientValues;
    vec3 diffuseValues;
    vec3 specularValues;
};

struct PointLight 
{
    vec3 lightPosition;
    
    float constantK;
    float linearK;
    float quadraticK;
	
    vec3 ambientValues;
    vec3 diffuseValues;
    vec3 specularValues;
};



in VS_OUT {
	vec3 outNormal;
	vec3 outFragPos;
	vec2 outTexCoords;
	vec4 outFragPosLightSpace;
	vec3 outTangentLightDir;
	vec3 outTangentViewPos;
	vec3 outTangent;
	vec3 outBiTangent;
    vec4 outDebugTangentColor;

} fragIns;

uniform vec3 viewPos;
uniform DirectionalLight dirLight;
uniform PointLight pointLight;
uniform sampler2D directionalShadowMap;
uniform float biasMin;
uniform float biasMax;
uniform float consK;
uniform float consM;
uniform Terrain terrainInstance;

vec3 calculateDirectionalLight(DirectionalLight dirLight, vec3 normal, vec3 viewDir, vec3 lightDirection, Terrain terrain, bool isSecondSet);
float calculateShadows(vec4 outFragPosLightSpace, vec3 lightDirection);
vec3 calculatePointLight(PointLight pointLight, vec3 normal, vec3 fragPos, vec3 viewDir, Terrain terrain, bool isSecondSet);

void main()
{
    vec3 normal;
    vec3 lightDirection;
    vec3 viewDir;

    normal = vec3(0.f, 0.f, 1.f); // Normal is (0, 0, 1) in tangent space
    lightDirection = normalize(fragIns.outTangentLightDir);
    viewDir = normalize(fragIns.outTangentViewPos);

    vec3 result1 = calculateDirectionalLight(dirLight, normal, viewDir, lightDirection, terrainInstance, false) + calculatePointLight(pointLight, normal, fragIns.outFragPos, viewDir, terrainInstance, false); 
    vec3 result2 = calculateDirectionalLight(dirLight, normal, viewDir, lightDirection, terrainInstance, true) + calculatePointLight(pointLight, normal, fragIns.outFragPos, viewDir, terrainInstance, true);

    float blendFactor = texture(terrainInstance.blendMap, fragIns.outTexCoords).r;
    vec4 intermFragColor = vec4(vec3(mix(result1, result2, blendFactor)), 1.f);

     float fogDensity = 0.00045f;
    float fogDistance = length(viewPos - fragIns.outFragPos);
    float fogHeight = exp(fragIns.outFragPos.y * fogDensity); // Increasing fog density at lower altitudes
    float fogAmount = 1.0 - exp(-fogDistance * fogDensity * fogHeight);

    // Only apply fog if fragment is beyond a certain distance or below a certain height
    if (fogDistance > 100.0 || fragIns.outFragPos.y < 10.0) {
        vec3 fogColor = vec3(0.24, 0.44, 0.45); 
        vec3 color = mix(vec3(intermFragColor), fogColor, fogAmount);
        FragColor = vec4(color, 1.f);
    } else {
        FragColor = vec4(vec3(intermFragColor), 1.f);
    }

    //FragColor = vec4(color, 1.f);
    
} 

vec3 calculateDirectionalLight(DirectionalLight dirLight, vec3 normal, vec3 viewDir, vec3 lightDirection, Terrain terrain, bool isSecondSet)
{ 
    vec3 baseColorMap1 = vec3(0.f);
    vec3 specularMap1 = vec3(0.f);
    vec3 tangentNormal1 = vec3(0.f);
    vec3 normal1;
    vec3 ambientC = vec3(0.f);
    vec3 diffuseC = vec3(0.f);
    float diffuse = 0.f;
    float roughness = 1.f;

    if(!isSecondSet)
    {
        baseColorMap1 = texture(terrain.diffuseMap1, fragIns.outTexCoords).rgb;

        tangentNormal1 = texture(terrain.normalMap1, fragIns.outTexCoords).rgb;
        tangentNormal1 = 2.0 * tangentNormal1 - vec3(1.0);
        normal1 = normalize(tangentNormal1);

        if(terrainInstance.hasSpecularMap)
        {
            specularMap1 = texture(terrain.specularMap1, fragIns.outTexCoords).rgb;
        }
        else
        {
            specularMap1 = terrainInstance.tSpecularValues;
        }
        
        float aoCoeff = texture(terrain.AOMap1, fragIns.outTexCoords).r;
        ambientC = dirLight.ambientValues * baseColorMap1 * aoCoeff;
        diffuse = max(dot(normal1, lightDirection), 0.f);
        //diffuseC = dirLight.diffuseValues * diffuse * baseColorMap * aoCoeff;
        diffuseC = dirLight.diffuseValues * diffuse * baseColorMap1 * aoCoeff;

        roughness = texture(terrain.roughnessMap1, fragIns.outTexCoords).r;
        terrain.shininess = (1.f - roughness) * consK + consM;
    }
    else
    {
        
        baseColorMap1 = texture(terrain.diffuseMap2, fragIns.outTexCoords).rgb;

        tangentNormal1 = texture(terrain.normalMap2, fragIns.outTexCoords).rgb;
        tangentNormal1 = 2.0 * tangentNormal1 - vec3(1.0);
        normal1 = normalize(tangentNormal1);

        if(terrainInstance.hasSpecularMap)
        {
            specularMap1 = texture(terrain.specularMap2, fragIns.outTexCoords).rgb;
        }
        else
        {
            specularMap1 = terrainInstance.tSpecularValues;
        }

        float aoCoeff = texture(terrain.AOMap2, fragIns.outTexCoords).r;
        ambientC = dirLight.ambientValues * baseColorMap1 * aoCoeff;
        diffuse = max(dot(normal1, lightDirection), 0.f);
        //diffuseC = dirLight.diffuseValues * diffuse * baseColorMap * aoCoeff;
        diffuseC = dirLight.diffuseValues * diffuse * baseColorMap1 * aoCoeff;

        roughness = texture(terrain.roughnessMap2, fragIns.outTexCoords).r;
        terrain.shininess = (1.f - roughness) * consK + consM;
    }

    /*Blinn Phong*/
    vec3 halfwayDir = normalize(lightDirection + viewDir);
    float specular = pow(max(dot(normal1, halfwayDir), 0.f),terrain.shininess);

    /*Shadow Component*/
    float shadow = calculateShadows(fragIns.outFragPosLightSpace, lightDirection);

    vec3 specularC = dirLight.specularValues * specular * specularMap1;

    return ambientC + ((1.0 - shadow) * (diffuseC + specularC));
};

float calculateShadows(vec4 fragPosLightSpace, vec3 lightDirection)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(directionalShadowMap, projCoords.xy).r; 
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // calculate bias (based on depth map resolution and slope)
    vec3 normal = normalize(fragIns.outNormal);
    float shadowBias = max(biasMax * (1.0 - dot(normal, lightDirection)), biasMin);
    // check whether current frag pos is in shadow
    // float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;
    // PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(directionalShadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(directionalShadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - shadowBias > pcfDepth  ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
    
    // keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if(projCoords.z > 1.0)
        shadow = 0.0;
        
    return shadow;

};

vec3 calculatePointLight(PointLight pointLight, vec3 normal, vec3 fragPos, vec3 viewDir, Terrain terrain, bool isSecondSet)
{
    vec3 lightDirection = normalize(pointLight.lightPosition - fragPos);

    vec3 baseColorMap;
    vec3 tangentNormal;
    vec3 specularMap;
    vec3 normalTerrain;
    float roughness;
    float aoCoeff;

    if(!isSecondSet)
    {
        baseColorMap = texture(terrain.diffuseMap1, fragIns.outTexCoords).rgb;

        tangentNormal = texture(terrain.normalMap1, fragIns.outTexCoords).rgb;
        tangentNormal = 2.0 * tangentNormal - vec3(1.0);
        normalTerrain = normalize(tangentNormal);

        if(terrainInstance.hasSpecularMap)
        {
            specularMap = texture(terrain.specularMap1, fragIns.outTexCoords).rgb;
        }
        else
        {
            specularMap = terrainInstance.tSpecularValues;
        }

        aoCoeff = texture(terrain.AOMap1, fragIns.outTexCoords).r;
        roughness = texture(terrain.roughnessMap1, fragIns.outTexCoords).r;
    }
    else
    {
        baseColorMap = texture(terrain.diffuseMap2, fragIns.outTexCoords).rgb;

        tangentNormal = texture(terrain.normalMap2, fragIns.outTexCoords).rgb;
        tangentNormal = 2.0 * tangentNormal - vec3(1.0);
        normalTerrain = normalize(tangentNormal);

        if(terrainInstance.hasSpecularMap)
        {
            specularMap = texture(terrain.specularMap2, fragIns.outTexCoords).rgb;
        }
        else
        {
            specularMap = terrainInstance.tSpecularValues;
        }

        aoCoeff = texture(terrain.AOMap2, fragIns.outTexCoords).r;
        roughness = texture(terrain.roughnessMap2, fragIns.outTexCoords).r;
    }

    vec3 ambient = pointLight.ambientValues * baseColorMap * aoCoeff;
    float diffuseStrength = max(dot(normalTerrain, lightDirection), 0.0);
    vec3 diffuse = pointLight.diffuseValues * diffuseStrength * baseColorMap;

    vec3 halfwayDir = normalize(lightDirection + viewDir);
    float specularStrength = pow(max(dot(normalTerrain, halfwayDir), 0.0), (1.0 - roughness) * 32.0 + 1.0);
    vec3 specular = pointLight.specularValues * specularStrength * specularMap;

    float distance = length(pointLight.lightPosition - fragPos);
    float attenuation = 1.0 / (pointLight.constantK + pointLight.linearK * distance + pointLight.quadraticK * (distance * distance));

    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;

    return ambient + diffuse + specular;
}