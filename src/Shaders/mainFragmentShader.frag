#version 330 core
out vec4 FragColor;

struct Material
{
    vec3 materialDiffuseValues;
    vec3 materialSpecularValues;
    vec3 materialEmissionValues;

    sampler2D diffuseMap;
    sampler2D specularMap;
    sampler2D normalMap;
    sampler2D AOMap;
    sampler2D roughnessMap;
    sampler2D emissiveMap;
    float emissionStr;
    float shininess;

    bool hasDiffuseMap;
    bool hasNormalMap;
    bool hasSpecularMap;
    bool hasAOMap;
    bool hasRoughnessMap;
    bool hasEmissiveMap;
    
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
uniform Material currentMaterial;
uniform DirectionalLight dirLight;
uniform PointLight pointLight;
uniform sampler2D directionalShadowMap;
uniform float biasMin;
uniform float biasMax;
uniform float consK;
uniform float consM;
uniform bool isPointLight = false;


vec3 calculateDirectionalLight(DirectionalLight dirLight, vec3 normal, vec3 viewDir, vec3 lightDirection, Material currentMaterial);
float calculateShadows(vec4 outFragPosLightSpace, vec3 lightDirection);
vec3 calculatePointLight(PointLight pointLight, vec3 normal, vec3 fragPos, vec3 viewDir, Material currentMaterial);

void main()
{
    if(isPointLight)
    {
        FragColor = vec4(1.f, 1.f, 1.f, 1.f);
    }
    else
    {
        vec3 normal;
        vec3 lightDirection;
        vec3 viewDir;

        if (currentMaterial.hasNormalMap) {
            // When the normal map is available, use tangent-space values
            normal = vec3(0.f, 0.f, 1.f); // Normal is (0, 0, 1) in tangent space
            lightDirection = normalize(fragIns.outTangentLightDir);
            viewDir = normalize(fragIns.outTangentViewPos);
        } else {
            // Without a normal map, use original values
            normal = normalize(fragIns.outNormal);
            lightDirection = normalize(dirLight.lightDirection);
            viewDir = normalize(viewPos - fragIns.outFragPos);


        }

        vec3 result = calculateDirectionalLight(dirLight, normal, viewDir, lightDirection, currentMaterial);
        result += calculatePointLight(pointLight, normal, fragIns.outFragPos, viewDir, currentMaterial);
        FragColor = vec4(result, 1.f);
    }
} 

vec3 calculateDirectionalLight(DirectionalLight dirLight, vec3 normal, vec3 viewDir, vec3 lightDirection, Material currentMaterial)
{ 
    vec3 baseColorMap = vec3(0.f);
    vec3 specularMap = vec3(0.f);
    vec3 tangentNormal = vec3(0.f);
    vec3 ambientC = vec3(0.f);
    vec3 diffuseC = vec3(0.f);
    float diffuse = 0.f;
    float roughness = 1.f;

    /*Check for Diffuse Map*/
    if(currentMaterial.hasDiffuseMap)
    {
        baseColorMap = texture(currentMaterial.diffuseMap, fragIns.outTexCoords).rgb;
    }
    else
    {
        baseColorMap = currentMaterial.materialDiffuseValues;
    }

    /*Check for Normal Map*/
    if(currentMaterial.hasNormalMap)
    {   
        tangentNormal = texture(currentMaterial.normalMap, fragIns.outTexCoords).rgb;
        tangentNormal = 2.0 * tangentNormal - vec3(1.0);
        normal = normalize(tangentNormal);
    }

    /*Check for Specular Map*/
    if(currentMaterial.hasSpecularMap)
    {
        specularMap = texture(currentMaterial.specularMap, fragIns.outTexCoords).rgb;
    }
    else
    {
        specularMap = currentMaterial.materialSpecularValues;
    }

    /*Check for AO Map*/
    if(currentMaterial.hasAOMap)
    {
        float aoCoeff = texture(currentMaterial.AOMap, fragIns.outTexCoords).r;
        ambientC = dirLight.ambientValues * baseColorMap * aoCoeff;
        diffuse = max(dot(normal, lightDirection), 0.f);
        //diffuseC = dirLight.diffuseValues * diffuse * baseColorMap * aoCoeff;
        diffuseC = dirLight.diffuseValues * diffuse * baseColorMap * aoCoeff;
    }
    else
    {
        ambientC = dirLight.ambientValues * baseColorMap;
        diffuse = max(dot(normal, lightDirection), 0.f);
        diffuseC = dirLight.diffuseValues * diffuse * baseColorMap;
    }

    /*Check for roughness map*/
    if(currentMaterial.hasRoughnessMap)
    {
        roughness = texture(currentMaterial.roughnessMap, fragIns.outTexCoords).r;
        currentMaterial.shininess = (1.f - roughness) * consK + consM;
    }


    /*Shadow Component*/
    float shadow = calculateShadows(fragIns.outFragPosLightSpace, lightDirection);

    /*Blinn-Phong*/
    vec3 halfwayDir = normalize(lightDirection + viewDir);
    float specular = pow(max(dot(normal, halfwayDir), 0.f),currentMaterial.shininess);
    vec3 specularC = dirLight.specularValues * specular * specularMap;

    if(currentMaterial.hasEmissiveMap)
    {
        vec3 emissionC = texture(currentMaterial.emissiveMap, fragIns.outTexCoords).rgb;
        return ambientC + ((1.0 - shadow) * (diffuseC + specularC)) + (emissionC * vec3(currentMaterial.emissionStr));
    }
    else
        return ambientC + ((1.0 - shadow) * (diffuseC + specularC));
        //return ambientC + diffuseC + specularC;
};

vec3 calculatePointLight(PointLight pointLight, vec3 normal, vec3 fragPos, vec3 viewDir, Material currentMaterial)
{
    vec3 lightDirection = normalize(pointLight.lightPosition - fragPos);

    vec3 baseColorMap = vec3(0.f);
    vec3 specularMap = vec3(0.f);
    vec3 tangentNormal = vec3(0.f);
    vec3 ambientC = vec3(0.f);
    vec3 diffuseC = vec3(0.f);
    float diffuse = 0.f;
    float roughness = 1.f;

    /*Check for Diffuse Map*/
    if(currentMaterial.hasDiffuseMap)
    {
        baseColorMap = texture(currentMaterial.diffuseMap, fragIns.outTexCoords).rgb;
    }
    else
    {
        baseColorMap = currentMaterial.materialDiffuseValues;
    }

    /*Check for Normal Map*/
    if(currentMaterial.hasNormalMap)
    {   
        tangentNormal = texture(currentMaterial.normalMap, fragIns.outTexCoords).rgb;
        tangentNormal = 2.0 * tangentNormal - vec3(1.0);
        normal = normalize(tangentNormal);
    }

    /*Check for Specular Map*/
    if(currentMaterial.hasSpecularMap)
    {
        specularMap = texture(currentMaterial.specularMap, fragIns.outTexCoords).rgb;
    }
    else
    {
        specularMap = currentMaterial.materialSpecularValues;
    }

    /*Check for AO Map*/
    if(currentMaterial.hasAOMap)
    {
        float aoCoeff = texture(currentMaterial.AOMap, fragIns.outTexCoords).r;
        ambientC = pointLight.ambientValues * baseColorMap * aoCoeff;
        diffuse = max(dot(normal, lightDirection), 0.f);
        diffuseC = pointLight.diffuseValues * diffuse * baseColorMap * aoCoeff;
    }
    else
    {
        ambientC = pointLight.ambientValues * baseColorMap;
        diffuse = max(dot(normal, lightDirection), 0.f);
        diffuseC = pointLight.diffuseValues * diffuse * baseColorMap;
    }

    /*Check for roughness map*/
    if(currentMaterial.hasRoughnessMap)
    {
        roughness = texture(currentMaterial.roughnessMap, fragIns.outTexCoords).r;
        currentMaterial.shininess = (1.f - roughness) * consK + consM;
    }

    /*Shadow Component*/
    //float shadow = calculateCubeMapShadows(fragIns.outFragPosLightSpace, lightDirection);

    /*Blinn-Phong*/
    vec3 reflectionDir = reflect(-lightDirection, normal);
    float specular = pow(max(dot(viewDir, reflectionDir), 0.f), currentMaterial.shininess);
    vec3 specularCoeff = pointLight.specularValues * specular * specularMap;

    /*Emissive map check*/
    vec3 emissionC = vec3(0.f);
    if(currentMaterial.hasEmissiveMap)
    {
        emissionC = texture(currentMaterial.emissiveMap, fragIns.outTexCoords).rgb * vec3(currentMaterial.emissionStr);
    }

    /*Apply attenuation*/
    float distance = length(pointLight.lightPosition - fragPos);
    float attenuation = 1.f / (pointLight.constantK + pointLight.linearK * distance + pointLight.quadraticK * (distance * distance));
    ambientC *= attenuation;
    diffuseC *= attenuation;
    specularCoeff *= attenuation;

    //return ambientC + ((1.0 - shadow) * (diffuseC + specularCoeff)) + emissionC;
    
    return ambientC + diffuseC + specularCoeff;
}



float calculateShadows(vec4 fragPosLightSpace, vec3 lightDirection)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    /*Depth from light perspective.*/
    projCoords = projCoords * 0.5 + 0.5;
    float closestDepth = texture(directionalShadowMap, projCoords.xy).r; 
    float currentDepth = projCoords.z;
    /*Calculate bias.*/
    vec3 normal = normalize(fragIns.outNormal);
    float shadowBias = max(biasMax * (1.0 - dot(normal, lightDirection)), biasMin);
    /*Check whether fragment is in shadow*/
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
    
    if(projCoords.z > 1.0)
        shadow = 0.0;
        
    return shadow;

};