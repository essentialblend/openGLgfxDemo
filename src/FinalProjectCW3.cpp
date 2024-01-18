#include <iostream>
#include <GLAD/glad.h>
#include <GLFWLib/glfw3.h>
#include "Headers/Shader.h"
#include "Headers/Camera.h"
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Headers/OBJParser.h"
#include "Headers/Vertex.h"
#include "Headers/PerlinHelper.h"
#include "Headers/PoissonHelper.h"
#include "Headers/Bitmap.h"
#include "Headers/VAO.h"
#include "Headers/VBO.h"
#include "Headers/EBO.h"
#include "Headers/FBO.h"
#include "Headers/RBO.h"

/*Function decl.*/
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void mouseCallback(GLFWwindow* window, double xPosInput, double yPosInput);
void setOBJModelBufferData(const std::unique_ptr<VAO>& objVAO, const std::unique_ptr<VBO>& objVBO, const std::unique_ptr<EBO>& objIBO, std::vector<Vertex>& outVertices,
    std::vector<unsigned int>& orderedIndices);
void renderDepthMapVizQuad();
void createDepthMapFBO(FBO* depthMapFBO, unsigned int& depthMapTexture, unsigned int shadowWidth, unsigned int shadowHeight);
void renderSceneForDepthMap(Shader& simpleDepthShader, glm::mat4& lightSpaceMatrix, const unsigned int& SHADOW_WIDTH, const unsigned int& SHADOW_HEIGHT, const std::unique_ptr<FBO>& depthMapFBO, glm::mat4& modelMat, const std::unique_ptr<VAO>& terrainVAO, std::vector<unsigned int>& terrainIndices, GLFWwindow& window, const std::unique_ptr<VAO>& towerVAO, std::vector<unsigned int>& towerIndices, const std::unique_ptr<VAO>& tower2VAO, std::vector<unsigned int>& tower2Indices, const std::unique_ptr<VAO>& tower3VAO, std::vector<unsigned int>& tower3Indices, const std::unique_ptr<VAO>& obeliskVAO, std::vector<unsigned int>& obeliskIndices, glm::vec3& depthMapLightPos, const std::unique_ptr<VAO>& octaVAO, std::vector<unsigned int>& octaIndices);
void generateTerrainBuffers(const std::unique_ptr<VAO>& terrainVAO, const std::unique_ptr<VBO>& terrainVBO, const std::unique_ptr<EBO>& terrainIBO, const std::vector<Vertex>& terrainVertices, const std::vector<unsigned int>& terrainIndices);
void setShaderUniforms(Shader& mainShader, glm::mat4& modelMat, glm::mat4& viewMat, glm::mat4& projMat, glm::vec3& viewPos, glm::mat4& lightSpaceMatrix, glm::vec3& lightDirection, float& biasMin, float& biasMax, float& sunAngle);
void setupDirectionVectorLine(unsigned int& lineVAO, unsigned int& lineVBO);
void updateDirectionVectorLine(unsigned int lineVBO, const glm::vec3& lightDirection);
void generateMainFramebufferWithFBOQuad(const std::unique_ptr<VAO>& fboQuadVAO, const std::unique_ptr<VBO>& fboQuadVBO, std::unique_ptr<FBO>& mainFBO, std::unique_ptr<RBO>& mainRBO, unsigned int& fboTex);
void generateOcclusionAndGodRaysFramebuffer(std::unique_ptr<FBO>& godRaysFBO, unsigned int& occlusionTexture);
void createSunBuffers(const std::unique_ptr<VAO>& sunVAO, const std::unique_ptr<VBO>& sunVBO, const std::unique_ptr<EBO>& sunEBO, std::vector<unsigned int>& sunIndices);
void renderSceneForGodRaysOcclusionMap(Shader& godRaysOcclusionShader, int& currentWidth, int& currentHeight, glm::mat4& projMat, glm::mat4& viewMat, glm::mat4& modelMat, glm::vec3& depthMapLightPos, const std::unique_ptr<VAO>& sunVAO, const std::unique_ptr<VAO>& towerVAO, const std::unique_ptr<VAO>& tower2VAO, const std::unique_ptr<VAO>& tower3VAO, std::vector<unsigned int>& towerIndices, std::vector<unsigned int>& tower2Indices, std::vector<unsigned int>& tower3Indices, const std::unique_ptr<VAO>& obeliskVAO, std::vector<unsigned int>& obeliskIndices, const std::unique_ptr<VAO>& terrainVAO, std::vector<unsigned int>& terrainIndices, glm::vec3& godRaysColor, const std::unique_ptr<FBO>& occlusionFBO);
std::vector<unsigned int> genPointLightOctahedronBuffers(const std::unique_ptr<VAO>& octaVAO, const std::unique_ptr<VBO>& octaVBO, const std::unique_ptr<EBO>& octaEBO);
unsigned int loadTextureFromBMP(const char* path);
unsigned int loadSRGBTextureFromBMP(const char* path);
unsigned int loadCubemap(std::vector<std::string>& cubemapFaces);


/*Helper functions*/
float hermite(float start, float end, float t)
{
    t = t * t * (3.0f - 2.0f * t);
    return start + t * (end - start);
};

glm::vec3 interpolateColors(const glm::vec3& colorStart, const glm::vec3& colorEnd, float sunAngle, float angleStart, float angleEnd)
{
    float t = (sunAngle - angleStart) / (angleEnd - angleStart);
    t = hermite(0.0f, 1.0f, t);
    return glm::mix(colorStart, colorEnd, t);
}

glm::vec3 srgbToLinear(glm::vec3 srgbColor, float gammaFactor)
{
    return glm::pow(srgbColor, glm::vec3(gammaFactor));
}

/*Error checking setup*/
/*Error check set up*/
void APIENTRY glDebugOutput(GLenum source,
    GLenum type,
    unsigned int id,
    GLenum severity,
    GLsizei length,
    const char* message,
    const void* userParam)
{
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return; // ignore these non-significant error codes

    std::cout << "---------------" << std::endl;
    std::cout << "Debug message (" << id << "): " << message << std::endl;

    switch (source)
    {
        case GL_DEBUG_SOURCE_API:
            std::cout << "Source: API"; 
            break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   
            std::cout << "Source: Window System"; 
            break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: 
            std::cout << "Source: Shader Compiler"; 
            break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:     
            std::cout << "Source: Third Party"; 
            break;
        case GL_DEBUG_SOURCE_APPLICATION:     
            std::cout << "Source: Application";
            break;
        case GL_DEBUG_SOURCE_OTHER:
            std::cout << "Source: Other"; 
            break;
    } std::cout << std::endl;

    switch (type)
    {
    case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Error"; break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behaviour"; break;
    case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
    case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: Performance"; break;
    case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
    case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
    case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
    case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
    } std::cout << std::endl;

    switch (severity)
    {
    case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
    case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
    case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
    case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
    } std::cout << std::endl;
    std::cout << std::endl;
}


/*Screen Resolution.*/
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

/*Time variables*/
float deltaTime = 0.f;
float lastFrame = 0.f;

/*Camera variables*/
Camera mainCamera(glm::vec3(0.f, 0.f, 10.f));
Camera stillCamera(glm::vec3(0.f, 5.f, 20.f));
float lastX = SCR_WIDTH / 2.0;
float lastY = SCR_HEIGHT / 2.0;
bool bfirstMouse = true;
bool usingStillCamera = false;

/*Shadow mapping variables*/
const unsigned int SHADOW_WIDTH = 2046, SHADOW_HEIGHT = 2046;
float nearPlane = 22.f, farPlane = 70.f;
float orthoLeftWidth = -28.5f;
float orthoRightWidth = 28.5f;
float orthoTopLength = 28.5f;
float orthoBotLength = -28.5f;

float biasMin = 0.0134f;
float biasMax = 0.0194f;
float tbiasMin = 0.00989f;
float tbiasMax = 0.0174f;

/*Directional Light variables*/
float orthoDistance = 50;
glm::vec3 dirLightDirection(-2, 4, -1);
float dirLightX = dirLightDirection.x;
float dirLightY = dirLightDirection.y;
float dirLightZ = dirLightDirection.z;
float angleX = 0.0f; // rotation around the X axis in radians
float angleY = 0.0f; // rotation around the Y axis in radians

/*Obelisk Emission Variables*/
float emissionR = 5.f;

/*Framebuffer variables.*/
unsigned int fboTex;
unsigned int occlusionTex;
unsigned int godRaysTex;
float grIntensity = 0.1f;
float grDecay = 0.1f;
float grDensity = 0.1f;
float grWeight = 0.1f;


/*Terrain and Perlin variables*/
const unsigned int PERLIN_SIZE = 256;
std::vector<int> perlinG{ 0 };
float hScale = 5.f;
float noiseScale = 2.f;
int terrainWidth = 50;
int terrainHeight = 50;
float fbmOctaves = 0.f;
float fbmPers = 0.f;
float consK = 20.f;
float consM = 20.f;

/*Building variables*/
/*Building 1*/
glm::vec3 towerBuilding1Location = glm::vec3(16.3999920, 0.799999595 - 3.f, 20.2000046);
glm::vec3 towerBuilding2Location = glm::vec3(3.19999981, 0.799999595 - 3.f, 20.2000046);
glm::vec3 towerBuilding3Location = glm::vec3(-9.99999239, 0.799999595 - 3.f, 20.2000046);

/*Building 2*/
glm::vec3 towerBuilding2Location1 = glm::vec3(5, -4.5999999990 - 3.5f, -22.4000130);
glm::vec3 towerBuilding2Location2 = glm::vec3(-15.7999897, -4.79999971 - 3.5f, -18.5999985);
glm::vec3 towerBuilding2Location3 = glm::vec3(-7.25273514, -2.59999967 - 3.5f, 18.6034470);
glm::vec3 towerBuilding2Location4 = glm::vec3(10.7999945, -2.60000038 - 3.5f, 19.2000008);
std::list<glm::vec3> tower2Locations = { towerBuilding2Location1, towerBuilding2Location2, towerBuilding2Location3, towerBuilding2Location4 };

/*Obelisk Hover variables*/
float obeliskTime = 0.0f;
const float obeliskAmplitude = 0.5f;
const float obeliskFrequency = 0.5f;
float globalRotationSpeed = 0.5f;
float globalTargetRotationSpeed = 0.5f;


/*Octahedron buffers*/
glm::vec3 octahedronPointLightPosition = glm::vec3(5.f, 5.f, 5.f);
float rotateTotalTime = 0.f;

/*Aux*/
std::unique_ptr<RBO> mainRBO;

int main()
{
    glfwInit();
    if (!glfwInit()) {
        std::cout << "Failed to initialize GLFW." << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 16);
    /*Enable debug context*/
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);

    /*Create the window context*/
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Coursework 3", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    if (glfwGetCurrentContext() == NULL) {
        std::cout << "Failed to make GLFW context current" << std::endl;
        return -1;
    }
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    /*Query debug context.*/
    int flags; glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
    {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(glDebugOutput, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
    }

    /*GL Version check*/
    std::cout << "GL VERSION: " << glGetString(GL_VERSION) << "\n";
    
    /*Callback functions*/
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    /*Enable Depth Testing*/
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);

    /*Antialiasing MSAA*/
    glEnable(GL_MULTISAMPLE);


    /*Set the seed.*/
    srand(static_cast<unsigned int>(time(NULL)));

    /*-----------------------PRE RENDER LOOP INIT----------------------*/

    /*------------------------SHADERS INIT----------------------------------*/
    Shader mainShader("src/Shaders/mainVertexShader.vert", "src/Shaders/mainFragmentShader.frag");
    
    /*SHADER FOR DEPTH MAP*/
    Shader simpleDepthShader("src/Shaders/simpleV.vert", "src/Shaders/simpleF.frag");
    
    Shader depthVizShader("src/Shaders/shadowMapV.vert", "src/Shaders/shadowMapF.frag");

    /*LIGHT REP SHADER*/
    Shader lightCubeShader("src/Shaders/lightRepVert.vert", "src/Shaders/lightRepFrag.frag");
        
    /*TERRAIN SHADER*/
    Shader terrainShader("src/Shaders/mainTerrainVertex.vert", "src/Shaders/mainTerrainFragment.frag");

    /*DIRECTION SHADER*/
    Shader directionVectorShader("src/Shaders/directionVertex.vert", "src/Shaders/directionFrag.frag");
    unsigned int lineVAO, lineVBO;
    setupDirectionVectorLine(lineVAO, lineVBO);

    /*POST PROCESS FBO SHADER*/
    Shader postProcessFBOShader("src/Shaders/postProcessVertex.vert", "src/Shaders/postProcessFragment.frag");

    /*GOD RAYS: OCCLUSION SHADER*/
    Shader godRaysOcclusionShader("src/Shaders/godRaysOcclusionVertex.vert", "src/Shaders/godRaysOcclusionFragment.frag");

    /*GOD RAYS: MAIN SHADER*/
    Shader mainGodRaysShader("src/Shaders/godRaysMainVertex.vert", "src/Shaders/godRaysMainFragment.frag");

    /*SKYBOX SHADER*/
    Shader skyboxShader("src/Shaders/skyboxVertex.vert", "src/Shaders/skyboxFragment.frag");

    /*--------------------------------------------------------------NIGHT SKYBOX INIT-------------------------------------------------------------------------------*/
    /*Vertices*/
    float skyboxVertices[] = {
        // positions          
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };
    
    /*Skybox VAO / VBO*/
    std::unique_ptr<VAO> skyboxVAO = std::make_unique<VAO>();
    std::unique_ptr<VBO> skyboxVBO = std::make_unique<VBO>();
    skyboxVAO->bind();
    skyboxVBO->bind();

    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    /*Skybox faces*/
    std::vector<std::string> nightSkyboxFaces
    {
        "dep/Skyboxes/Night/posx.bmp",
        "dep/Skyboxes/Night/negx.bmp",
        "dep/Skyboxes/Night/posy.bmp",
        "dep/Skyboxes/Night/negy.bmp",
        "dep/Skyboxes/Night/posz.bmp",
        "dep/Skyboxes/Night/negz.bmp"
    };

    /*Skybox faces*/
    std::vector<std::string> daySkyboxFaces
    {
        "dep/Skyboxes/Day/posx.bmp",
        "dep/Skyboxes/Day/negx.bmp",
        "dep/Skyboxes/Day/posy.bmp",
        "dep/Skyboxes/Day/negy.bmp",
        "dep/Skyboxes/Day/posz.bmp",
        "dep/Skyboxes/Day/negz.bmp"
    };

    /*Cubemap textures*/
    unsigned int nightCubemapTexture = loadCubemap(nightSkyboxFaces);
    unsigned int dayCubemapTexture = loadCubemap(daySkyboxFaces);


    /*--------------------------------------------TERRAIN VERTICES AND BUFFER GENERATION---------------------------------------------------------*/
    std::vector<unsigned int> terrainIndices;
    std::vector<Vertex> terrainVertices;
    float outMinHeight = 0.f;
    float outMaxHeight = 0.f;
    int seed = 0;
    perlinNoiseInit(perlinG, seed);
    generateTerrainVerticesIndices(terrainWidth, terrainHeight, hScale, terrainVertices, terrainIndices, perlinG, outMinHeight, outMaxHeight);

    /*OpenGL Objects*/
    std::unique_ptr<VAO> terrainVAO = std::make_unique<VAO>();
    std::unique_ptr<VBO> terrainVBO = std::make_unique<VBO>();
    std::unique_ptr<EBO> terrainEBO = std::make_unique<EBO>();

    generateTerrainBuffers(terrainVAO, terrainVBO, terrainEBO, terrainVertices, terrainIndices);

    std::vector<float> blendMapArray = generateBlendMap(terrainVertices, terrainWidth, terrainHeight, outMinHeight, outMaxHeight);
    mainCamera.setBlendMap(blendMapArray);

    // Generate the OpenGL texture object
    GLuint blendMapTexture;
    glGenTextures(1, &blendMapTexture);
    glBindTexture(GL_TEXTURE_2D, blendMapTexture);

    // Give the image to OpenGL
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, terrainWidth, terrainHeight, 0, GL_RED, GL_FLOAT, blendMapArray.data());

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Generate mipmaps
    glGenerateMipmap(GL_TEXTURE_2D);

    // Unbind the texture
    glBindTexture(GL_TEXTURE_2D, 0);
    /*-------------------------------------------------------------------------------------------------------------------------------------------*/
    
    /*------------------------------------------------- QUAD FOR FRAMEBUFFER---------------------------------------------------------------------*/
    
    std::unique_ptr<VAO> fboQuadVAO = std::make_unique<VAO>();
    std::unique_ptr<VBO> fboQuadVBO = std::make_unique<VBO>();
    std::unique_ptr<FBO> mainFBO = std::make_unique<FBO>();
    mainRBO = std::make_unique<RBO>();


    generateMainFramebufferWithFBOQuad(fboQuadVAO, fboQuadVBO, mainFBO, mainRBO, fboTex);
    /*-------------------------------------------------------------------------------------------------------------------------------------------*/

    /*-----------------------------------FBOs FOR GODRAYS / OCCLUSION---------------------------------------------*/
    
    std::unique_ptr<FBO> godRaysOcclusionFBO = std::make_unique<FBO>();
    std::unique_ptr<FBO> godRaysMainFBO = std::make_unique<FBO>();

    generateOcclusionAndGodRaysFramebuffer(godRaysOcclusionFBO, occlusionTex);
    generateOcclusionAndGodRaysFramebuffer(godRaysMainFBO, godRaysTex);


    /*HAND WRITTEN VERTICES*/



    /*----------------------------------------------------------OBJ PARSER-----------------------------------------------------------------------*/

    /*---------------------------------TOWER BUILDING OBJ--------------------------------*/
    std::vector<Vertex> towerVertices;
    std::vector<unsigned int> towerIndices;
    std::map<std::tuple<GLuint, GLuint, GLuint>, GLuint> towerUnique;

    if (!parseOBJFile("dep/SceneBuildings/towerBuilding.obj", towerVertices, towerIndices, towerUnique))
    {
        std::cout << "OBJ PARSER: FAILED TO LOAD OBJ MODEL\n";
    }

    std::unique_ptr<VAO> towerBuilding1VAO = std::make_unique<VAO>();
    std::unique_ptr<VBO> towerBuilding1VBO = std::make_unique<VBO>();
    std::unique_ptr<EBO> towerBuilding1EBO = std::make_unique<EBO>();

    
    setOBJModelBufferData(towerBuilding1VAO, towerBuilding1VBO, towerBuilding1EBO, towerVertices, towerIndices);

    unsigned int towerDiffuseMap = loadSRGBTextureFromBMP("dep/SceneBuildings/towerBuilding.bmp");
    /*----------------------------------------------------------------------*/
    /*---------------------------TOWER BUILDING 2------------------------------------------*/
    std::vector<Vertex> tower2Vertices;
    std::vector<unsigned int> tower2Indices;
    std::map<std::tuple<GLuint, GLuint, GLuint>, GLuint> tower2Unique;

    if (!parseOBJFile("dep/SceneBuildings/towerBuilding2.obj", tower2Vertices, tower2Indices, tower2Unique))
    {
        std::cout << "OBJ PARSER: FAILED TO LOAD OBJ MODEL\n";
    }

    std::unique_ptr<VAO> towerBuilding2VAO = std::make_unique<VAO>();
    std::unique_ptr<VBO> towerBuilding2VBO = std::make_unique<VBO>();
    std::unique_ptr<EBO> towerBuilding2EBO = std::make_unique<EBO>();
    
    setOBJModelBufferData(towerBuilding2VAO, towerBuilding2VBO, towerBuilding2EBO, tower2Vertices, tower2Indices);

    /*---------------------------TOWER BUILDING 3------------------------------------------*/
    std::vector<Vertex> tower3Vertices;
    std::vector<unsigned int> tower3Indices;
    std::map<std::tuple<GLuint, GLuint, GLuint>, GLuint> tower3Unique;

    if (!parseOBJFile("dep/SceneBuildings/towerBuilding3.obj", tower3Vertices, tower3Indices, tower3Unique))
    {
        std::cout << "OBJ PARSER: FAILED TO LOAD OBJ MODEL\n";
    }

    std::unique_ptr<VAO> towerBuilding3VAO = std::make_unique<VAO>();
    std::unique_ptr<VBO> towerBuilding3VBO = std::make_unique<VBO>();
    std::unique_ptr<EBO> towerBuilding3EBO = std::make_unique<EBO>();
    setOBJModelBufferData(towerBuilding3VAO, towerBuilding3VBO, towerBuilding3EBO, tower3Vertices, tower3Indices);
    
    /*------------------------------------------------------------------------------------------------------------*/

    /*--------------------------MAIN OBJ PARSER MODEL: OBELISK---------------------------------*/
    std::vector<Vertex> obeliskVertices;
    std::vector<unsigned int> obeliskIndices;
    std::map<std::tuple<GLuint, GLuint, GLuint>, GLuint> obeliskUnique;

    if (!parseOBJFile("dep/MainObelisk/obelisk.obj", obeliskVertices, obeliskIndices, obeliskUnique))
    {
        std::cout << "OBJ PARSER: FAILED TO LOAD OBJ MODEL\n";
    }

    std::unique_ptr<VAO> obeliskVAO = std::make_unique<VAO>();
    std::unique_ptr<VBO> obeliskVBO = std::make_unique<VBO>();
    std::unique_ptr<EBO> obeliskEBO = std::make_unique<EBO>();
    setOBJModelBufferData(obeliskVAO, obeliskVBO, obeliskEBO, obeliskVertices, obeliskIndices);

    unsigned int obeliskDiffuse = loadSRGBTextureFromBMP("dep/Compressed/obeliskDiffuse.bmp");
    unsigned int obeliskEmissive = loadSRGBTextureFromBMP("dep/Compressed/obeliskEmissive.bmp");
    unsigned int obeliskNormals = loadTextureFromBMP("dep/Compressed/obeliskNormals.bmp");
    unsigned int obeliskRoughness = loadTextureFromBMP("dep/Compressed/obeliskRoughness.bmp");
    
    /*------------------------------------------------------------------*/
    /*-----------------------------------SUN BILLBOARD------------------------*/
    std::unique_ptr<VAO> sunBillboardVAO = std::make_unique<VAO>();
    std::unique_ptr<VBO> sunBillboardVBO = std::make_unique<VBO>();
    std::unique_ptr<EBO> sunBillboardEBO = std::make_unique<EBO>();
    std::vector<unsigned int> sunIndices = {
    0, 1, 2,  // first Triangle
    0, 2, 3   // second Triangle
    };
    createSunBuffers(sunBillboardVAO, sunBillboardVBO, sunBillboardEBO, sunIndices);

    /*------------------------------------------------------------------------*/

    /*-----------------------------------HAND-WRITTEN OCTAHEDRON------------------------*/
    
    /*----------------------------------------------------------------------------------*/

    /*-------------------------------------------------------------DEPTH MAP FBO FOR SHADOW MAPPING----------------------------------------------*/
    std::unique_ptr<FBO> depthMapFBO = std::make_unique<FBO>();
    unsigned int depthMapTexture = 0;

    createDepthMapFBO(depthMapFBO.get(), depthMapTexture, SHADOW_WIDTH, SHADOW_HEIGHT);
    /*---------------------------------------------------------------------------------------------*/
    
    /*TEXTURES*/
    unsigned int floorDiffuseMap = loadSRGBTextureFromBMP("dep/TerrainCompressed/mudFloor1Diffuse.bmp");
    unsigned int floorNormalMap = loadTextureFromBMP("dep/TerrainCompressed/mudFloor1Normal.bmp");
    unsigned int floorAOMap = loadTextureFromBMP("dep/TerrainCompressed/mudFloor1AO.bmp");
    unsigned int floorRoughnessMap = loadTextureFromBMP("dep/TerrainCompressed/mudFloor1Roughness.bmp");


    unsigned int floorDiffuseMap2 = loadSRGBTextureFromBMP("dep/TerrainCompressed/mudFloor2Diffuse.bmp");
    unsigned int floorNormalMap2 = loadTextureFromBMP("dep/TerrainCompressed/mudFloor2Normal.bmp");
    unsigned int floorAOMap2 = loadTextureFromBMP("dep/TerrainCompressed/mudFloor2AO.bmp");
    unsigned int floorRoughnessMap2 = loadTextureFromBMP("dep/TerrainCompressed/mudFloor2Roughness.bmp");

    float sunRotateTime = 0.f;

    /*OCTAHEDRON BUFFERS INIT.*/
    std::unique_ptr<VAO> octaVAO = std::make_unique<VAO>();
    std::unique_ptr<VBO> octaVBO = std::make_unique<VBO>();
    std::unique_ptr<EBO> octaEBO = std::make_unique<EBO>();

    std::vector<unsigned int> octaIndices = genPointLightOctahedronBuffers(octaVAO, octaVBO, octaEBO);

    while (!glfwWindowShouldClose(window))
    {
        /*------------------------------TOTAL INIT----------------------------*/
        
        /*Time variables*/
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        /*Process input and flush buffers.*/
        processInput(window);

        glClearColor(0.f, 0.f, 0.f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);

        /*--------------------------------------------------------------PRE-CALCULATIONS------------------------------------------------------------*/
        /*DIR LIGHT */
        sunRotateTime += 0.0001f;
        float sunAngle = glm::cos(sunRotateTime * glm::pi<float>()) * 90;
        //float sunAngle = 90.f;

        glm::vec3 sunDir;
        sunDir.x = 0.f;
        sunDir.y = glm::cos(glm::radians(sunAngle));
        sunDir.z = -glm::sin(glm::radians(sunAngle));

        dirLightDirection = sunDir;
        glm::vec3 depthMapLightPos = glm::normalize(dirLightDirection) * orthoDistance;


        /*Modulate Godrays color*/
        glm::vec3 godRaysColor;
        glm::vec3 minColor = glm::vec3(0.5, 0.5, 0.5);
        glm::vec3 nightColor = glm::vec3(0.1f, 0.1f, 0.1f);
        nightColor = glm::max(srgbToLinear(nightColor, 2.4f), minColor);
        glm::vec3 sunriseColor = glm::vec3(0.8f, 0.4f, 0.3f);
        sunriseColor = glm::max(srgbToLinear(sunriseColor, 2.4f), minColor);
        glm::vec3 earlyMorningColor = glm::vec3(0.9f, 0.7f, 0.5f);
        earlyMorningColor = glm::max(srgbToLinear(earlyMorningColor, 2.4f), minColor);
        glm::vec3 lateMorningColor = glm::vec3(1.0f, 0.9f, 0.8f);
        lateMorningColor = glm::max(srgbToLinear(lateMorningColor, 2.4f), minColor);
        glm::vec3 noonColor = glm::vec3(1.0f, 1.0f, 0.9f);
        noonColor = glm::max(srgbToLinear(noonColor, 2.4f), minColor);
        glm::vec3 earlyAfternoonColor = glm::vec3(1.0f, 0.9f, 0.8f);
        earlyAfternoonColor = glm::max(srgbToLinear(earlyAfternoonColor, 2.4f), minColor);
        glm::vec3 lateAfternoonColor = glm::vec3(0.9f, 0.7f, 0.5f);
        lateAfternoonColor = glm::max(srgbToLinear(lateAfternoonColor, 2.4f), minColor);
        glm::vec3 sunsetColor = glm::vec3(0.9f, 0.5f, 0.4f);
        sunsetColor = glm::max(srgbToLinear(sunsetColor, 2.4f), minColor);
        glm::vec3 duskColor = glm::vec3(0.6f, 0.3f, 0.3f);
        duskColor = glm::max(srgbToLinear(duskColor, 2.4f), minColor);

        if (sunAngle >= -85.0f && sunAngle < -45.0f) {
            godRaysColor = interpolateColors(nightColor, sunriseColor, sunAngle, -85.0f, -45.0f);
        }
        else if (sunAngle >= -45.0f && sunAngle < 0.0f) {
            godRaysColor = interpolateColors(sunriseColor, noonColor, sunAngle, -45.0f, 0.0f);
        }
        else if (sunAngle >= 0.0f && sunAngle < 45.0f) {
            godRaysColor = interpolateColors(noonColor, lateAfternoonColor, sunAngle, 0.0f, 45.0f);
        }
        else if (sunAngle >= 45.0f && sunAngle < 75.0f) {
            godRaysColor = interpolateColors(lateAfternoonColor, sunsetColor, sunAngle, 45.0f, 75.0f);
        }
        else if (sunAngle >= 75.0f && sunAngle <= 85.0f) {
            godRaysColor = interpolateColors(sunsetColor, nightColor, sunAngle, 75.0f, 85.0f);
        }
        else {
            godRaysColor = nightColor;
        }

        godRaysColor = glm::clamp(godRaysColor, 0.0f, 1.0f);



        /*----------------------------------------------------------------------------------------------------------*/
        /*--------------------------BASE VIEW AND PROJ MATRICES FOR MAIN CAMERA---------------------------------------------------*/
        /*View and Projection Matrices*/
        glm::mat4 lightSpaceMatrix(glm::mat4(1.f));
        glm::mat4 modelMat(glm::mat4(1.f));
        int currentWidth{ 0 }, currentHeight{ 0 };
        glfwGetFramebufferSize(window, &currentWidth, &currentHeight);
        glm::mat4 projMat = glm::mat4(1.f);
        glm::mat4 viewMat = glm::mat4(1.f);
        if (usingStillCamera) {
            projMat = glm::perspective(glm::radians(stillCamera.zoom), (float)currentWidth / (float)currentHeight, 0.1f, 500.f);
            viewMat = stillCamera.getViewMatrix();
        }
        else {
            projMat = glm::perspective(glm::radians(mainCamera.zoom), (float)currentWidth / (float)currentHeight, 0.1f, 500.f);
            viewMat = mainCamera.getViewMatrix();
        }

        /*--------------------------------------------------------------------*/


        /*-----------------------------------------RENDER DEPTH OF THE SCENE FOR SHADOW MAPPING--------------------------------------------------*/
        


        renderSceneForDepthMap(simpleDepthShader, lightSpaceMatrix, SHADOW_WIDTH, SHADOW_HEIGHT, depthMapFBO, modelMat, terrainVAO, terrainIndices, *window, towerBuilding1VAO, towerIndices, towerBuilding2VAO, tower2Indices, towerBuilding3VAO, tower3Indices, obeliskVAO, obeliskIndices, depthMapLightPos, octaVAO, octaIndices);
        /*-----------------------------------------------------------------*/

        /*----------------------------RENDER OCCLUSION PASS FOR GODRAYS---------------------------------------------*/

        renderSceneForGodRaysOcclusionMap(godRaysOcclusionShader, currentWidth, currentHeight, projMat, viewMat, modelMat, depthMapLightPos, sunBillboardVAO, towerBuilding1VAO, towerBuilding2VAO, towerBuilding3VAO, towerIndices, tower2Indices, tower3Indices, obeliskVAO, obeliskIndices, terrainVAO, terrainIndices, godRaysColor, godRaysOcclusionFBO);

        /*------------------------------------------------------------MAIN RENDER TO POST PROCESS FBO----------------------------------------------------------------*/
        mainFBO->bind();
        glClearColor(0.f, 0.f, 0.f, 1.f);
        glViewport(0, 0, currentWidth, currentHeight);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glm::vec3 materialAmbientValues = srgbToLinear(glm::vec3(1.f, 1.f, 1.f), 2.2f);
        glm::vec3 materialSpecularValues = glm::vec3(0.5f, 0.5f, 0.5f);
        glm::vec3 pointLightAmbientValues = glm::vec3(0.35f, 0.035f, 0.35f);
        glm::vec3 pointLightDiffuseValues = glm::vec3(0.8f, 0.8f, 0.8f);
        glm::vec3 pointLightSpecularValues = glm::vec3(1.0f, 1.0f, 1.0f);



        /*TOWER BUILDING 1*/
        setShaderUniforms(mainShader, modelMat, viewMat, projMat, mainCamera.Position, lightSpaceMatrix, dirLightDirection, biasMin, biasMax, sunAngle);
        mainShader.setBool("currentMaterial.hasDiffuseMap", true);
        mainShader.setBool("currentMaterial.hasNormalMap", false);
        mainShader.setBool("currentMaterial.hasSpecularMap", false);
        mainShader.setBool("currentMaterial.hasRoughnessMap", false);
        mainShader.setBool("currentMaterial.hasEmissiveMap", false);
        mainShader.setBool("currentMaterial.hasAOMap", false);
        mainShader.setFloat("currentMaterial.shininess", 32);
        mainShader.setInt("directionalShadowMap", 1);
        mainShader.setInt("currentMaterial.diffuseMap", 2);
        mainShader.setVec3("currentMaterial.materialAmbientValues", materialAmbientValues);
        mainShader.setVec3("currentMaterial.materialSpecularValues", materialSpecularValues);
        mainShader.setFloat("consK", consK);
        mainShader.setFloat("consM", consM);
        mainShader.setFloat("biasMin", biasMin);
        mainShader.setFloat("biasMax", biasMax);
        mainShader.setBool("isInstanced", false);
        mainShader.setVec3("pointLight.lightPosition", octahedronPointLightPosition);
        mainShader.setVec3("pointLight.ambientValues", pointLightAmbientValues);
        mainShader.setVec3("pointLight.diffuseValues", pointLightDiffuseValues);
        mainShader.setVec3("pointLight.specularValues", pointLightSpecularValues);
        mainShader.setFloat("pointLight.constantK", 1.0f);
        mainShader.setFloat("pointLight.linearK", 0.09f);
        mainShader.setFloat("pointLight.quadraticK", 0.02f);

        towerBuilding1VAO->bind();
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMapTexture);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, towerDiffuseMap);

        for (int i = 0; i < 3; i++)
        {
            modelMat = glm::mat4(1.f);
            if (i == 0)
                modelMat = glm::translate(modelMat, towerBuilding1Location);
            if (i == 1)
            {
                modelMat = glm::translate(modelMat, towerBuilding2Location);
            }
            if (i == 2)
            {
                modelMat = glm::translate(modelMat, towerBuilding3Location);
            }
            modelMat = glm::scale(modelMat, glm::vec3(0.5f));
            mainShader.setMat4("modelMat", modelMat);
            glDrawElements(GL_TRIANGLES, GLsizei(towerIndices.size()), GL_UNSIGNED_INT, nullptr);
        }

        /*------------------------------------------TOWER BUILDING 2-----------------------------------------*/

        towerBuilding2VAO->bind();
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMapTexture);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, towerDiffuseMap);

        for (glm::vec3 location : tower2Locations)
        {
            modelMat = glm::mat4(1.f);
            modelMat = glm::translate(modelMat, location);
            modelMat = glm::scale(modelMat, glm::vec3(0.5f));
            glm::mat4 rotationY = glm::rotate(glm::mat4(1.0f), float(-1.59999883), glm::vec3(0.0f, 1.0f, 0.0f));
            modelMat = rotationY * modelMat;
            mainShader.setMat4("modelMat", modelMat);
            glDrawElements(GL_TRIANGLES, GLsizei(tower2Indices.size()), GL_UNSIGNED_INT, nullptr);
        }

        /*--------------------------------------TOWER BUILDING 3-----------------------*/
        towerBuilding3VAO->bind();
        modelMat = glm::mat4(1.f);
        // [3] = {x=0.619141638 y=2.00000000 z=-21.7912159 }
        modelMat = glm::translate(modelMat, glm::vec3(0.619141638, 2.00000000 - 1.5f, 21.7912159));
        modelMat = glm::scale(modelMat, glm::vec3(0.5f));
        // Create rotation matrices
        glm::mat4 rotationY = glm::rotate(glm::mat4(1.0f), float(-3.16999745), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMat = rotationY * modelMat;
        mainShader.setMat4("modelMat", modelMat);
        glDrawElements(GL_TRIANGLES, GLsizei(tower3Indices.size()), GL_UNSIGNED_INT, nullptr);

        /*----------------------------------Obelisk----------------------------*/
        modelMat = glm::mat4(1.f);
        float yOffset = obeliskAmplitude * sin(obeliskFrequency * obeliskTime);
        //[3] = {x=0.199999854 y=10.9999943 z=0.00000000 }
        modelMat = glm::translate(modelMat, glm::vec3(0.199999854, 10.9999943 + yOffset, 0.00000000));
        glm::vec3 obeliskPos = modelMat[3];
        obeliskTime += deltaTime;
        modelMat = glm::scale(modelMat, glm::vec3(2.f));
        setShaderUniforms(mainShader, modelMat, viewMat, projMat, mainCamera.Position, lightSpaceMatrix, dirLightDirection, biasMin, biasMax, sunAngle);
        mainShader.setBool("currentMaterial.hasDiffuseMap", true);
        mainShader.setBool("currentMaterial.hasNormalMap", true);
        mainShader.setBool("currentMaterial.hasSpecularMap", false);
        mainShader.setBool("currentMaterial.hasRoughnessMap", true);
        mainShader.setBool("currentMaterial.hasEmissiveMap", true);
        mainShader.setBool("currentMaterial.hasAOMap", false);
        mainShader.setFloat("currentMaterial.shininess", 32);
        mainShader.setInt("currentMaterial.diffuseMap", 11);
        mainShader.setInt("currentMaterial.normalMap", 12);
        mainShader.setInt("currentMaterial.roughnessMap", 13);
        mainShader.setInt("currentMaterial.emissiveMap", 14);
        mainShader.setVec3("currentMaterial.materialSpecularValues", 0.4f, 0.4f, 0.4f);
        mainShader.setFloat("pointLight.constantK", 1.0f);
        mainShader.setFloat("pointLight.linearK", 0.09f);
        mainShader.setFloat("pointLight.quadraticK", 0.02f);

        float speedFactor = 0.5f; // Adjust speed of changes here
        float timeValue = glfwGetTime();
        float sinValue = sin(timeValue * speedFactor); // Output between -1 and 1
        float emissionStrength = sinValue * 6.0f + 4.0f; // Output between -2 and 10

        mainShader.setFloat("currentMaterial.emissionStr", emissionStrength);


        mainShader.setBool("isInstanced", false);
        obeliskVAO->bind();
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMapTexture);
        glActiveTexture(GL_TEXTURE11);
        glBindTexture(GL_TEXTURE_2D, obeliskDiffuse);
        glActiveTexture(GL_TEXTURE12);
        glBindTexture(GL_TEXTURE_2D, obeliskNormals);
        glActiveTexture(GL_TEXTURE13);
        glBindTexture(GL_TEXTURE_2D, obeliskRoughness);
        glActiveTexture(GL_TEXTURE14);
        glBindTexture(GL_TEXTURE_2D, obeliskEmissive);
        glDrawElements(GL_TRIANGLES, GLsizei(obeliskIndices.size()), GL_UNSIGNED_INT, nullptr);


       /*TERRAIN*/
        modelMat = glm::mat4(1.f);
        setShaderUniforms(terrainShader, modelMat, viewMat, projMat, mainCamera.Position, lightSpaceMatrix, dirLightDirection, biasMin, biasMax,sunAngle);
        terrainShader.setBool("isInstanced", false);
        terrainShader.setFloat("consK", consK);
        terrainShader.setFloat("consM", consM);
        terrainShader.setFloat("biasMin", tbiasMin);
        terrainShader.setFloat("biasMax", tbiasMax);
        terrainShader.setInt("directionalShadowMap", 1);
        terrainShader.setInt("terrainInstance.diffuseMap1", 2);
        terrainShader.setInt("terrainInstance.normalMap1", 3);
        terrainShader.setInt("terrainInstance.AOMap1", 4);
        terrainShader.setInt("terrainInstance.roughnessMap1", 5);
        terrainShader.setInt("terrainInstance.diffuseMap2", 6);
        terrainShader.setInt("terrainInstance.normalMap2", 7);
        terrainShader.setInt("terrainInstance.AOMap2", 8);
        terrainShader.setInt("terrainInstance.roughnessMap2", 9);
        terrainShader.setFloat("terrainInstance.blendFactor", 10);
        terrainShader.setBool("terrainInstance.hasSpecularMap", false);
        terrainShader.setVec3("terrainInstance.tSpecularValues", 0.2f, 0.2f, 0.2f);
        terrainShader.setFloat("terrainInstance.shininess", 32);
        terrainShader.setVec3("pointLight.lightPosition", octahedronPointLightPosition);
        terrainShader.setVec3("pointLight.ambientValues", pointLightAmbientValues);
        terrainShader.setVec3("pointLight.diffuseValues", pointLightDiffuseValues);
        terrainShader.setVec3("pointLight.specularValues", pointLightSpecularValues);
        terrainShader.setFloat("pointLight.constantK", 1.0f);
        terrainShader.setFloat("pointLight.linearK", 0.09f);
        terrainShader.setFloat("pointLight.quadraticK", 0.02f);

        terrainShader.setFloat("uTime", currentFrame);
        terrainShader.setFloat("noiseScale", noiseScale);
        terrainShader.setFloat("heightScale", hScale);
        terrainVAO->bind();
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMapTexture);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, floorDiffuseMap);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, floorNormalMap);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, floorAOMap);
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, floorRoughnessMap);
        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_2D, floorDiffuseMap2);
        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_2D, floorNormalMap2);
        glActiveTexture(GL_TEXTURE8);
        glBindTexture(GL_TEXTURE_2D, floorAOMap2);
        glActiveTexture(GL_TEXTURE9);
        glBindTexture(GL_TEXTURE_2D, floorRoughnessMap2);
        glActiveTexture(GL_TEXTURE10);
        glBindTexture(GL_TEXTURE_2D, blendMapTexture);
        glDrawElements(GL_TRIANGLES, GLsizei(terrainIndices.size()), GL_UNSIGNED_INT, nullptr);

        /* Octahedron */
        rotateTotalTime += deltaTime;
        float rotationSpeed = globalRotationSpeed;
        
        const float rotationAcceleration = 0.2f * deltaTime;

        // Move rotationSpeed towards the target speed
        if (globalRotationSpeed < globalTargetRotationSpeed) {
            globalRotationSpeed += rotationAcceleration;
            if (globalRotationSpeed > globalTargetRotationSpeed) globalRotationSpeed = globalTargetRotationSpeed; // Ensure we don't overshoot
        }
        else if (globalRotationSpeed > globalTargetRotationSpeed) {
            globalRotationSpeed -= rotationAcceleration;
            if (globalRotationSpeed < globalTargetRotationSpeed) globalRotationSpeed = globalTargetRotationSpeed; // Ensure we don't overshoot
        }

        // Compute rotation around the obelisk
        float rotationAngle = globalRotationSpeed * rotateTotalTime;
        glm::mat4 rotationMat = glm::rotate(glm::mat4(1.0f), rotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::vec3 relativeLightPosition = glm::vec3(15.f, 0.f, 0.f);
        glm::vec4 rotatedRelativePosition = rotationMat * glm::vec4(relativeLightPosition, 1.0f);
        glm::vec3 basePosition = glm::vec3(rotatedRelativePosition) + obeliskPos;

        // Initial position (P0) based on rotation
        glm::vec3 P0 = basePosition;

        // Two random control points (for demonstration, chosen arbitrarily)
        glm::vec3 P1 = obeliskPos + glm::vec3(0.0f, 5.0f, 0.0f);  // somewhat above the obelisk
        glm::vec3 P2 = obeliskPos + glm::vec3(5.0f, 0.0f, 0.0f);  // somewhat to the right of the obelisk

        // Bezier calculation
        float t = (sin(rotateTotalTime) + 1.0f) * 0.5f;  // This will oscillate t between 0 and 1
        glm::vec3 bezierOffset = (1 - t) * (1 - t) * glm::vec3(0) + 2 * (1 - t) * t * P1 + t * t * P2 - P0;

        // Reduce the magnitude of the Bezier offset and add a multiplicative factor
        float bezierFactor = 0.5f; // Adjust this as needed
        bezierOffset *= bezierFactor;

        // Apply the Bezier offset to the base position
        octahedronPointLightPosition = basePosition + bezierOffset;
        modelMat = glm::mat4(1.f);
        modelMat = glm::translate(modelMat, octahedronPointLightPosition);
        modelMat = glm::scale(modelMat, glm::vec3(2.f));
        setShaderUniforms(mainShader, modelMat, viewMat, projMat, mainCamera.Position, lightSpaceMatrix, dirLightDirection, biasMin, biasMax, sunAngle);
        mainShader.setBool("currentMaterial.hasDiffuseMap", false);
        mainShader.setBool("currentMaterial.hasNormalMap", false);
        mainShader.setBool("currentMaterial.hasSpecularMap", false);
        mainShader.setBool("currentMaterial.hasRoughnessMap", false);
        mainShader.setBool("currentMaterial.hasEmissiveMap", false);
        mainShader.setBool("currentMaterial.hasAOMap", false);
        mainShader.setFloat("currentMaterial.shininess", 32);
        mainShader.setVec3("currentMaterial.materialAmbientValues", 0.4f , 0.4f, 0.4f);
        mainShader.setVec3("currentMaterial.materialDiffuseValues", 1.f, 0.95f, 1.f);
        mainShader.setVec3("currentMaterial.materialSpecularValues", 0.4f, 0.4f, 0.4f);
        mainShader.setBool("isInstanced", false);
        mainShader.setBool("isPointLight", true);
        mainShader.setVec3("pointLight.lightPosition", octahedronPointLightPosition);

        octaVAO->bind();
        GLsizei numberOfIndices = static_cast<GLsizei>(octaIndices.size());
        glDrawElements(GL_TRIANGLES, numberOfIndices, GL_UNSIGNED_INT, 0);
        
        mainShader.setBool("isPointLight", false);
        float factor = glm::clamp((dirLightDirection.y + 0.2f) * 0.25f, 0.0f, 1.0f);

        // Skybox, last.
        glDepthFunc(GL_LEQUAL);
        skyboxShader.UseShader();
        viewMat = glm::mat4(glm::mat3(mainCamera.getViewMatrix()));
        skyboxShader.setMat4("viewMat", viewMat);
        skyboxShader.setMat4("projMat", projMat);
        skyboxShader.setInt("nightSkyboxCubemap", 0);
        skyboxShader.setInt("daySkyboxCubemap", 1);
        skyboxShader.setFloat("interpFactor", factor);

        skyboxVAO->bind();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, nightCubemapTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, dayCubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS);

/*--------------------------------------------------------------------------------------------------------------------------------------*/

        /*----------------------------RENDER FOR GODRAYS RADIUS BLUR-------------------*/
        godRaysMainFBO->bind();
        glViewport(0, 0, currentWidth, currentHeight);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);




        mainGodRaysShader.UseShader();
        mainGodRaysShader.setInt("GRtDiffuse", 11);
        mainGodRaysShader.setFloat("GRExposure", 1.2f);
        mainGodRaysShader.setFloat("GRDecay", float(0.959999442f)); //grDecay = 0.959999442
        mainGodRaysShader.setFloat("GRDensity", 1.48999882f); //grDensity = 1.48999882
        mainGodRaysShader.setFloat("GRWeight", 0.9f); //grWeight = 0.999999523
        mainGodRaysShader.setInt("GRSamples", 0);
        mainGodRaysShader.setVec3("GRColor", godRaysColor);

        glm::vec4 lightPositionClipSpace = projMat * viewMat * glm::vec4(depthMapLightPos, 1.0f);
        lightPositionClipSpace /= lightPositionClipSpace.w;
        glm::vec2 lightPositionNDC = (glm::vec2(lightPositionClipSpace.x, lightPositionClipSpace.y) + 1.0f) * 0.5f;
        mainGodRaysShader.setVec2("GRlightPosition", lightPositionNDC);

        glActiveTexture(GL_TEXTURE11);
        glBindTexture(GL_TEXTURE_2D, occlusionTex); // bind the occlusion texture

        fboQuadVAO->bind();
        glDrawArrays(GL_TRIANGLES, 0, 6);


        /*----------------------------------------------------------------------*/
        
        /*------------------------------BIND DEFAULT FBO AND RENDER TO QUAD--------------------------------------------------------*/
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        //glEnable(GL_FRAMEBUFFER_SRGB);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glClearColor(0.f, 0.f, 0.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        /*Modulate godrays intensity according to time of day.*/
        float godRaysIntensity;

        /*Night to Sunrise*/
        if (sunAngle >= -80.0f && sunAngle < -45.0f) {
            float t = (sunAngle + 80.0f) / 35.0f; // normalize t to be between 0 and 1
            godRaysIntensity = glm::mix(0.2f, 0.55f, t);
        }
        /*Sunrise to Noon*/
        else if (sunAngle >= -45.0f && sunAngle < 45.0f) {
            float t = (sunAngle + 45.0f) / 90.0f; // normalize t to be between 0 and 1
            godRaysIntensity = glm::mix(0.55f, 0.4f, t);
        }
        /*Noon to Sunset*/
        else if (sunAngle >= 45.0f && sunAngle < 80.0f) {
            float t = (sunAngle - 45.0f) / 35.0f; // normalize t to be between 0 and 1
            godRaysIntensity = glm::mix(0.4f, 0.55f, t);
        }
        /*Sunset to Early Night*/
        else if (sunAngle >= 80.0f && sunAngle < 90.0f) {
            float t = (sunAngle - 80.0f) / 10.0f; // normalize t to be between 0 and 1
            godRaysIntensity = glm::mix(0.55f, 0.2f, t);
        }
        /*Night*/
        else {
            godRaysIntensity = 0.2f;
        }

        postProcessFBOShader.UseShader();
        postProcessFBOShader.setInt("mainSceneTexture", 12);
        postProcessFBOShader.setInt("godRaysTexture", 13);
        postProcessFBOShader.setFloat("godRaysIntensity", 0.f);

        glActiveTexture(GL_TEXTURE12);
        glBindTexture(GL_TEXTURE_2D, fboTex);
        glActiveTexture(GL_TEXTURE13);
        glBindTexture(GL_TEXTURE_2D, godRaysTex);
        fboQuadVAO->bind(); // Use your existing VAO for rendering the quad
        glDrawArrays(GL_TRIANGLES, 0, 6);

        //glDisable(GL_FRAMEBUFFER_SRGB);
        
        /*Depth map viz*/
        //glBindFramebuffer(GL_FRAMEBUFFER, 0);
        //glClearColor(1.f, 1.f, 1.f, 1.f);
        //glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //depthVizShader.UseShader();
        //glActiveTexture(GL_TEXTURE0);
        //glBindTexture(GL_TEXTURE_2D, depthMapTexture); // Replace with the correct ID
        //depthVizShader.setInt("depthMap", 0);
        //depthVizShader.setFloat("nearPlane", nearPlane);
        //depthVizShader.setFloat("farPlane", farPlane);
        //fboQuadVAO->bind(); // Use your existing VAO for rendering the quad
        //glDrawArrays(GL_TRIANGLES, 0, 6);


/*-------------------------------------------------------------------------------------------------------------------------------------*/

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    {
        mainCamera.processKeyboardInput(FORWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    {
        mainCamera.processKeyboardInput(BACKWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    {
        mainCamera.processKeyboardInput(LEFT, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    {
        mainCamera.processKeyboardInput(RIGHT, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
    {
        globalTargetRotationSpeed = 5.f;
        if (!usingStillCamera)
        {
            usingStillCamera = true;
        }
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_RELEASE && usingStillCamera)
    {
        usingStillCamera = false;
        globalTargetRotationSpeed = 0.5f;
    }
    if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS)
    {
        stillCamera.Position.y -= 0.2f;
        std::cout << stillCamera.Position.y << "\n";
    }
    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS)
    {
        stillCamera.Position.y += 0.2f;
        std::cout << stillCamera.Position.y << "\n";
    }

}

/*Framebuffer size callback.*/
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);

    // Resize framebuffer
    glBindTexture(GL_TEXTURE_2D, fboTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Resize occlusion texture
    glBindTexture(GL_TEXTURE_2D, occlusionTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Resize Godrays texture
    glBindTexture(GL_TEXTURE_2D, godRaysTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Resize renderbuffer
    mainRBO->bind();
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

/*Mouse callback.*/
void mouseCallback(GLFWwindow* window, double xPosInput, double yPosInput)
{
    float xPos = static_cast<float>(xPosInput);
    float yPos = static_cast<float>(yPosInput);

    if (bfirstMouse)
    {
        lastX = xPos;
        lastY = yPos;
        bfirstMouse = false;
    }

    float xOffset = xPos - lastX;
    float yOffset = lastY - yPos;
    lastX = xPos;
    lastY = yPos;

    mainCamera.processMouseMovement(xOffset, yOffset);
}

void setOBJModelBufferData(const std::unique_ptr<VAO>& objVAO, const std::unique_ptr<VBO>& objVBO, const std::unique_ptr<EBO>& objIBO, std::vector<Vertex>& outVertices,
    std::vector<unsigned int>& orderedIndices)
{
    objVAO->bind();

    objVBO->bind();
    glBufferData(GL_ARRAY_BUFFER, outVertices.size() * sizeof(Vertex), outVertices.data(), GL_STATIC_DRAW);

    objIBO->bind();
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, orderedIndices.size() * sizeof(unsigned int), orderedIndices.data(), GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, vPos));
    glEnableVertexAttribArray(0);

    // Texture coordinate attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, vTexCoords));
    glEnableVertexAttribArray(1);

    // Normal attribute
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, vNormals)));
    glEnableVertexAttribArray(2);

    // Normal attribute
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, vTangent)));
    glEnableVertexAttribArray(3);

    // Normal attribute
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, vBiTangent)));
    glEnableVertexAttribArray(4);
    
    // Unbind VBO and VAO
    objVAO->unbind();
    objVBO->unbind();
    objIBO->unbind();
}

// TEMP QUAD FOR VIZ DEPTH MAP.
// -----------------------------------------
unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderDepthMapVizQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void createDepthMapFBO(FBO* depthMapFBO, unsigned int& depthMapTexture, unsigned int shadowWidth, unsigned int shadowHeight)
{
    glGenTextures(1, &depthMapTexture);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadowWidth, shadowHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    // Attach depth texture to framebuffer.
    depthMapFBO->bind();
    GLenum err1;
    while ((err1 = glGetError()) != GL_NO_ERROR) {
        std::cerr << "OpenGL error after createDepthMap bind: " << err1 << std::endl;
    }
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);

    // Check for OpenGL errors after attaching the texture
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cerr << "OpenGL error after glFramebufferTexture2D: " << err << std::endl;
    }

    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    // Check if the framebuffer is complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cerr << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    }

    depthMapFBO->unbind();
}

void renderSceneForDepthMap(Shader& simpleDepthShader, glm::mat4& lightSpaceMatrix, const unsigned int& SHADOW_WIDTH, const unsigned int& SHADOW_HEIGHT, const std::unique_ptr<FBO>& depthMapFBO, glm::mat4& modelMat, const std::unique_ptr<VAO>& terrainVAO, std::vector<unsigned int>& terrainIndices, GLFWwindow& window, const std::unique_ptr<VAO>& towerVAO, std::vector<unsigned int>& towerIndices, const std::unique_ptr<VAO>& tower2VAO, std::vector<unsigned int>& tower2Indices, const std::unique_ptr<VAO>& tower3VAO, std::vector<unsigned int>& tower3Indices, const std::unique_ptr<VAO>& obeliskVAO, std::vector<unsigned int>& obeliskIndices, glm::vec3& depthMapLightPos, const std::unique_ptr<VAO>& octaVAO, std::vector<unsigned int>& octaIndices)
{
    simpleDepthShader.UseShader();
    glm::mat4 lightProjection, lightView;
    lightProjection = glm::ortho(orthoLeftWidth, orthoRightWidth, orthoBotLength, orthoTopLength, nearPlane, farPlane);
    lightView = glm::lookAt(depthMapLightPos, glm::vec3(0.f), glm::vec3(0.0, 1.0, 0.0));
    lightSpaceMatrix = lightProjection * lightView;

    depthMapFBO->bind();
    glClear(GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);

    /*RENDER FOR DEPTH MAP*/
    modelMat = glm::mat4(1.f);
    modelMat = glm::scale(modelMat, glm::vec3(0.1f));
    simpleDepthShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
    simpleDepthShader.setMat4("modelMat", modelMat);

    /*TOWER 1*/
    towerVAO->bind();
    for (int i = 0; i < 3; i++)
    {
        modelMat = glm::mat4(1.f);
        if (i == 0)
            modelMat = glm::translate(modelMat, towerBuilding1Location);
        if (i == 1)
        {
            modelMat = glm::translate(modelMat, towerBuilding2Location);
        }
        if (i == 2)
        {
            modelMat = glm::translate(modelMat, towerBuilding3Location);
        }
        modelMat = glm::scale(modelMat, glm::vec3(0.5f));
        simpleDepthShader.setMat4("modelMat", modelMat);
        glDrawElements(GL_TRIANGLES, GLsizei(towerIndices.size()), GL_UNSIGNED_INT, nullptr);
    }

    /*TOWER 2*/
    tower2VAO->bind();
    for (glm::vec3 location : tower2Locations)
    {
        modelMat = glm::mat4(1.f);
        modelMat = glm::translate(modelMat, location);
        modelMat = glm::scale(modelMat, glm::vec3(0.5f));
        glm::mat4 rotationY = glm::rotate(glm::mat4(1.0f), float(-1.59999883), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMat = rotationY * modelMat;
        simpleDepthShader.setMat4("modelMat", modelMat);
        glDrawElements(GL_TRIANGLES, GLsizei(tower2Indices.size()), GL_UNSIGNED_INT, nullptr);
    }

    /*TOWER 3*/
    tower3VAO->bind();
    modelMat = glm::mat4(1.f);
    // [3] = {x=0.619141638 y=2.00000000 z=-21.7912159 }
    modelMat = glm::translate(modelMat, glm::vec3(0.619141638, 2.00000000 - 1.5f, 21.7912159));
    modelMat = glm::scale(modelMat, glm::vec3(0.5f));
    // Create rotation matrices
    glm::mat4 rotationY = glm::rotate(glm::mat4(1.0f), float(-3.16999745), glm::vec3(0.0f, 1.0f, 0.0f));
    modelMat = rotationY * modelMat;
    simpleDepthShader.setMat4("modelMat", modelMat);
    glDrawElements(GL_TRIANGLES, GLsizei(tower3Indices.size()), GL_UNSIGNED_INT, nullptr);

    /*OBELISK*/
    obeliskVAO->bind();
    modelMat = glm::mat4(1.f);
    float yOffset = obeliskAmplitude * sin(obeliskFrequency * obeliskTime);
    //[3] = {x=0.199999854 y=10.9999943 z=0.00000000 }
    modelMat = glm::translate(modelMat, glm::vec3(0.199999854, 10.9999943 + yOffset, 0.00000000));
    obeliskTime += deltaTime;
    modelMat = glm::scale(modelMat, glm::vec3(2.f));
    simpleDepthShader.setMat4("modelMat", modelMat);
    glDrawElements(GL_TRIANGLES, GLsizei(obeliskIndices.size()), GL_UNSIGNED_INT, nullptr);

    /*TERRAIN*/
    modelMat = glm::mat4(1.f);
    simpleDepthShader.setMat4("modelMat", modelMat);
    terrainVAO->bind();
    glDrawElements(GL_TRIANGLES, GLsizei(terrainIndices.size()), GL_UNSIGNED_INT, nullptr);

    /*Octahedron*/
    modelMat = glm::mat4(1.f);
    modelMat = glm::translate(modelMat, octahedronPointLightPosition);
    modelMat = glm::scale(modelMat, glm::vec3(2.f));
    simpleDepthShader.setMat4("modelMat", modelMat);
    octaVAO->bind();
    GLsizei numberOfIndices = static_cast<GLsizei>(octaIndices.size());
    glDrawElements(GL_TRIANGLES, numberOfIndices, GL_UNSIGNED_INT, 0);


    depthMapFBO->unbind();

    /*RESET VIEWPORT*/
    int currentWidth{ 0 }, currentHeight{ 0 };
    glfwGetFramebufferSize(&window, &currentWidth, &currentHeight);
    glViewport(0, 0, currentWidth, currentHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void generateTerrainBuffers(const std::unique_ptr<VAO>& terrainVAO, const std::unique_ptr<VBO>& terrainVBO, const std::unique_ptr<EBO>& terrainIBO, const std::vector<Vertex>& terrainVertices, const std::vector<unsigned int>& terrainIndices)
{
    terrainVAO->bind();

    terrainVBO->bind();

    glBufferData(GL_ARRAY_BUFFER, terrainVertices.size() * sizeof(Vertex), terrainVertices.data(), GL_STATIC_DRAW);

    terrainIBO->bind();
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, terrainIndices.size() * sizeof(unsigned int), terrainIndices.data(), GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, vPos));
    glEnableVertexAttribArray(0);

    // Texture coordinate attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, vTexCoords));
    glEnableVertexAttribArray(1);

    // Normal attribute
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, vNormals)));
    glEnableVertexAttribArray(2);

    // Normal attribute
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, vTangent)));
    glEnableVertexAttribArray(3);

    // Normal attribute
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, vBiTangent)));
    glEnableVertexAttribArray(4);
    
    // Unbind VBO and VAO
    terrainVAO->unbind();
    terrainVBO->unbind();
    terrainIBO->unbind();
}

void setShaderUniforms(Shader& mainShader, glm::mat4& modelMat, glm::mat4& viewMat, glm::mat4& projMat, glm::vec3& viewPos, glm::mat4& lightSpaceMatrix, glm::vec3& lightDirection, float& biasMin, float& biasMax, float& sunAngle)
{
    glm::vec3 ambient{ 0 };
    glm::vec3 diffuse{ 0 };
    glm::vec3 specular{ 0 };

    mainShader.UseShader();
    
    mainShader.setMat4("modelMat", modelMat);
    mainShader.setMat4("viewMat", viewMat);
    mainShader.setMat4("projMat", projMat);
    mainShader.setVec3("viewPos", mainCamera.Position);
    mainShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
    mainShader.setVec3("vertexLightDirection", lightDirection);
    mainShader.setVec3("dirLight.lightDirection", lightDirection);

    /*Interpolate the directional light color based on time of day.*/
    float factor = (lightDirection.y + 1.f) / 2.f;

    // Sunrise colors and values
    glm::vec3 ambientSunrise = glm::vec3(0.15f, 0.075f, 0.075f); // Slightly dim reddish ambient light
    glm::vec3 diffuseSunrise = glm::vec3(0.9f, 0.5f, 0.3f);   // Strong reddish-orange diffuse light
    glm::vec3 specularSunrise = glm::vec3(0.8f, 0.7f, 0.6f);  // Warm specular light

    // Noon colors and values
    glm::vec3 ambientNoon = glm::vec3(0.3f, 0.3f, 0.35f);     // Bright blueish ambient light
    glm::vec3 diffuseNoon = glm::vec3(0.5f, 0.5f, 0.5f);      // Very bright, almost white diffuse light
    glm::vec3 specularNoon = glm::vec3(1.0f, 0.95f, 0.9f);    // Bright specular light

    // Sunset colors and values
    glm::vec3 ambientSunset = glm::vec3(0.15f, 0.075f, 0.075f);  // Similar to sunrise
    glm::vec3 diffuseSunset = glm::vec3(0.9f, 0.4f, 0.3f);    // Strong reddish diffuse light
    glm::vec3 specularSunset = glm::vec3(0.8f, 0.6f, 0.5f);   // Warm specular light

    // Night-time colors and values
    glm::vec3 ambientNight = glm::vec3(0.01f, 0.01f, 0.05f);   // Very dim blueish ambient light
    glm::vec3 diffuseNight = glm::vec3(0.0f, 0.0f, 0.1f);     // Almost no diffuse light
    glm::vec3 specularNight = glm::vec3(0.25f, 0.25f, 0.3f);    // Dim, cold specular light

    /*Dir light transition*/
    if (sunAngle >= -85.0f && sunAngle < -45.0f) {
        float t = (sunAngle + 85.0f) / 40.0f;
        ambient = mix(ambientNight, ambientSunrise, t);
        diffuse = mix(diffuseNight, diffuseSunrise, t);
        specular = mix(specularNight, specularSunrise, t);
    }
    else if (sunAngle >= -45.0f && sunAngle < 45.0f) {
        float t = (sunAngle + 45.0f) / 90.0f;
        ambient = mix(ambientSunrise, ambientNoon, t);
        diffuse = mix(diffuseSunrise, diffuseNoon, t);
        specular = mix(specularSunrise, specularNoon, t);
    }
    else if (sunAngle >= 45.0f && sunAngle < 75.0f) {
        float t = (sunAngle - 45.0f) / 30.0f;
        ambient = mix(ambientNoon, ambientSunset, t);
        diffuse = mix(diffuseNoon, diffuseSunset, t);
        specular = mix(specularNoon, specularSunset, t);
    }
    else if (sunAngle >= 75.0f && sunAngle <= 85.0f) {
        float t = (sunAngle - 75.0f) / 10.0f;
        ambient = mix(ambientSunset, ambientNight, t);
        diffuse = mix(diffuseSunset, diffuseNight, t);
        specular = mix(specularSunset, specularNight, t);
    }
    else {
        ambient = ambientNight;
        diffuse = diffuseNight;
        specular = specularNight;
    }

    //ambient = srgbToLinear(ambient, 2.2f);
    //diffuse = srgbToLinear(diffuse, 2.2f);
    //specular = srgbToLinear(specular, 2.2f);

    mainShader.setVec3("dirLight.ambientValues", ambient);
    mainShader.setVec3("dirLight.diffuseValues", diffuse);
    mainShader.setVec3("dirLight.specularValues", specular);

    mainShader.setFloat("biasMin", static_cast<float>(biasMin));
    mainShader.setFloat("biasMax", static_cast<float>(biasMax));
}

void setupDirectionVectorLine(unsigned int& lineVAO, unsigned int& lineVBO)
{
    float vertices[] = {
        0.f, 0.f, 0.f,
        1.f, 1.f, 1.f
    };

    glGenVertexArrays(1, &lineVAO);
    glGenBuffers(1, &lineVBO);
    glBindVertexArray(lineVAO);

    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void updateDirectionVectorLine(unsigned int lineVBO, const glm::vec3& lightDirection)
{
    glm::vec3 inverse = -lightDirection;
    float vertices[] = {
        0.f, 0.f, 0.f,
        inverse.x, inverse.y, inverse.z
    };

    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void generateMainFramebufferWithFBOQuad(const std::unique_ptr<VAO>& fboQuadVAO, const std::unique_ptr<VBO>& fboQuadVBO, std::unique_ptr<FBO>& mainFBO, std::unique_ptr<RBO>& mainRBO, unsigned int& fboTex)
{
    float quadVertices[] = {
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };
    fboQuadVAO->bind();
    fboQuadVBO->bind();
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    fboQuadVAO->unbind();
    fboQuadVBO->unbind();

    /*Main Framebuffer*/
    mainFBO->bind();
    glGenTextures(1, &fboTex);
    glBindTexture(GL_TEXTURE_2D, fboTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fboTex, 0);

    /*Main Renderbuffer*/
    mainRBO->bind();
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, mainRBO->getRBO());
    /*Check Framebuffer completeness*/
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    }

    mainFBO->unbind();
    mainRBO->unbind();
}

void generateOcclusionAndGodRaysFramebuffer(std::unique_ptr<FBO>& godRaysFBO, unsigned int& occlusionTexture)
{
    /*Occlusion Framebuffer*/
    godRaysFBO->bind();

    /*Occlusion Texture*/
    glGenTextures(1, &occlusionTexture);
    glBindTexture(GL_TEXTURE_2D, occlusionTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, occlusionTexture, 0);

    /*Check Framebuffer completeness*/
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    }

    godRaysFBO->unbind();
}

void createSunBuffers(const std::unique_ptr<VAO>& sunVAO, const std::unique_ptr<VBO>& sunVBO, const std::unique_ptr<EBO>& sunEBO, std::vector<unsigned int>& sunIndices)
{
    float sunVertices[] = {
        // positions   // texCoords
        -0.5f,  0.5f, 0.0f, 0.0f, 1.0f,
         0.5f,  0.5f, 0.0f, 1.0f, 1.0f,
         0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
        -0.5f, -0.5f, 0.0f, 0.0f, 0.0f
    };

    sunVAO->bind();
    sunVBO->bind();
    glBufferData(GL_ARRAY_BUFFER, sizeof(sunVertices), sunVertices, GL_STATIC_DRAW);

    sunEBO->bind();
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sunIndices.size() * sizeof(unsigned int), sunIndices.data(), GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // TexCoord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    sunVAO->unbind();
    sunVBO->unbind();
    sunEBO->unbind();
}

void renderSceneForGodRaysOcclusionMap(Shader& godRaysOcclusionShader, int& currentWidth, int& currentHeight, glm::mat4& projMat, glm::mat4& viewMat, glm::mat4& modelMat, glm::vec3& depthMapLightPos, const std::unique_ptr<VAO>& sunVAO, const std::unique_ptr<VAO>& towerVAO, const std::unique_ptr<VAO>& tower2VAO, const std::unique_ptr<VAO>& tower3VAO, std::vector<unsigned int>& towerIndices, std::vector<unsigned int>& tower2Indices, std::vector<unsigned int>& tower3Indices, const std::unique_ptr<VAO>& obeliskVAO, std::vector<unsigned int>& obeliskIndices, const std::unique_ptr<VAO>& terrainVAO, std::vector<unsigned int>& terrainIndices, glm::vec3& godRaysColor, const std::unique_ptr<FBO>& occlusionFBO)
{
    occlusionFBO->bind();
    glViewport(0, 0, currentWidth, currentHeight);
    glClearColor(0.4f, 0.4f, 0.4f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    godRaysOcclusionShader.UseShader();

    godRaysOcclusionShader.setMat4("projMat", projMat);
    godRaysOcclusionShader.setMat4("viewMat", viewMat);

    /*Render sun billboard*/
    modelMat = glm::mat4(1.f);

    glm::vec3 lightDirection = glm::normalize(depthMapLightPos - glm::vec3(0.0f));

    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);  // up vector
    glm::vec3 right = glm::normalize(glm::cross(up, lightDirection));
    up = glm::normalize(glm::cross(lightDirection, right));

    glm::mat4 rotation = glm::mat4(glm::vec4(right, 0.0f),
        glm::vec4(up, 0.0f),
        glm::vec4(lightDirection, 0.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));



    modelMat = glm::translate(modelMat, depthMapLightPos);
    modelMat *= rotation;
    modelMat = glm::scale(modelMat, glm::vec3(100.f));
    godRaysOcclusionShader.setMat4("modelMat", modelMat);
    godRaysOcclusionShader.setBool("isSun", true);
    godRaysOcclusionShader.setVec3("godRaysColor", godRaysColor);
    sunVAO->bind();
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glEnable(GL_BLEND);

    /*Render Tower Buildings*/
    /*Tower 1*/
    godRaysOcclusionShader.setBool("isSun", false);
    towerVAO->bind();
    for (int i = 0; i < 3; i++)
    {
        modelMat = glm::mat4(1.f);
        if (i == 0)
            modelMat = glm::translate(modelMat, towerBuilding1Location);
        if (i == 1)
        {
            modelMat = glm::translate(modelMat, towerBuilding2Location);
        }
        if (i == 2)
        {
            modelMat = glm::translate(modelMat, towerBuilding3Location);
        }
        modelMat = glm::scale(modelMat, glm::vec3(0.5f));
        godRaysOcclusionShader.setMat4("modelMat", modelMat);
        glDrawElements(GL_TRIANGLES, GLsizei(towerIndices.size()), GL_UNSIGNED_INT, nullptr);
    }

    /*Tower 2*/
    godRaysOcclusionShader.setBool("isSun", false);
    tower2VAO->bind();
    for (glm::vec3 location : tower2Locations)
    {
        modelMat = glm::mat4(1.f);
        modelMat = glm::translate(modelMat, location);
        modelMat = glm::scale(modelMat, glm::vec3(0.5f));
        glm::mat4 rotationY = glm::rotate(glm::mat4(1.0f), float(-1.59999883), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMat = rotationY * modelMat;
        godRaysOcclusionShader.setMat4("modelMat", modelMat);
        glDrawElements(GL_TRIANGLES, GLsizei(tower2Indices.size()), GL_UNSIGNED_INT, nullptr);
    }

    /*Tower 3*/
    godRaysOcclusionShader.setBool("isSun", false);
    tower3VAO->bind();
    modelMat = glm::mat4(1.f);
    // [3] = {x=0.619141638 y=2.00000000 z=-21.7912159 }
    modelMat = glm::translate(modelMat, glm::vec3(0.619141638, 2.00000000 - 1.5f, 21.7912159));
    modelMat = glm::scale(modelMat, glm::vec3(0.5f));
    // Create rotation matrices
    glm::mat4 rotationY = glm::rotate(glm::mat4(1.0f), float(-3.16999745), glm::vec3(0.0f, 1.0f, 0.0f));
    modelMat = rotationY * modelMat;
    godRaysOcclusionShader.setMat4("modelMat", modelMat);
    glDrawElements(GL_TRIANGLES, GLsizei(tower3Indices.size()), GL_UNSIGNED_INT, nullptr);

    /*Render obelisk*/
    godRaysOcclusionShader.setBool("isSun", false);
    obeliskVAO->bind();
    modelMat = glm::mat4(1.f);
    float yOffset = obeliskAmplitude * sin(obeliskFrequency * obeliskTime);
    //[3] = {x=0.199999854 y=10.9999943 z=0.00000000 }
    modelMat = glm::translate(modelMat, glm::vec3(0.199999854, 10.9999943 + yOffset, 0.00000000));
    obeliskTime += deltaTime;
    modelMat = glm::scale(modelMat, glm::vec3(2.f));
    godRaysOcclusionShader.setMat4("modelMat", modelMat);
    glDrawElements(GL_TRIANGLES, GLsizei(obeliskIndices.size()), GL_UNSIGNED_INT, nullptr);

    /*Render terrain*/
    godRaysOcclusionShader.setBool("isSun", false);
    terrainVAO->bind();
    modelMat = glm::mat4(1.f);
    godRaysOcclusionShader.setMat4("modelMat", modelMat);

    glDrawElements(GL_TRIANGLES, GLsizei(terrainIndices.size()), GL_UNSIGNED_INT, nullptr);

    occlusionFBO->unbind();
}
std::vector<unsigned int> genPointLightOctahedronBuffers(const std::unique_ptr<VAO>& octaVAO, const std::unique_ptr<VBO>& octaVBO, const std::unique_ptr<EBO>& octaEBO)
{
    std::vector<unsigned int> octahedronIndices =
    {
        // Top Pyramid
        0, 1, 2,
        0, 2, 3,
        0, 3, 4,
        0, 4, 1,
        // Bottom Pyramid
        5, 6, 7,
        5, 7, 8,
        5, 8, 9,
        5, 9, 6,
    };

    float octahedronVertices[] =
    {
        // Positions           // Texture Coords  // Normals          
        // Top Pyramid
        0.0f,  1.0f,  0.0f,    0.5f, 1.0f,       0.0f,  1.0f,  0.0f,
       -1.0f,  0.0f, -1.0f,    0.0f, 0.0f,      -1.0f,  0.0f, -1.0f,
        1.0f,  0.0f, -1.0f,    1.0f, 0.0f,       1.0f,  0.0f, -1.0f,
        1.0f,  0.0f,  1.0f,    1.0f, 0.0f,       1.0f,  0.0f,  1.0f,
       -1.0f,  0.0f,  1.0f,    0.0f, 0.0f,      -1.0f,  0.0f,  1.0f,
       // Bottom Pyramid
       0.0f, -1.0f,  0.0f,    0.5f, 1.0f,       0.0f, -1.0f,  0.0f,
      -1.0f,  0.0f, -1.0f,    0.0f, 0.0f,      -1.0f,  0.0f, -1.0f,
       1.0f,  0.0f, -1.0f,    1.0f, 0.0f,       1.0f,  0.0f, -1.0f,
       1.0f,  0.0f,  1.0f,    1.0f, 0.0f,       1.0f,  0.0f,  1.0f,
      -1.0f,  0.0f,  1.0f,    0.0f, 0.0f,      -1.0f,  0.0f,  1.0f,
    };

    octaVAO->bind();
    octaVBO->bind();
    glBufferData(GL_ARRAY_BUFFER, sizeof(octahedronVertices), octahedronVertices, GL_STATIC_DRAW);

    octaEBO->bind();
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, octahedronIndices.size() * sizeof(unsigned int), octahedronIndices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);

    octaVAO->unbind();
    return octahedronIndices;
}

unsigned int loadTextureFromBMP(const char* path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;

    FILE* file;
    fopen_s(&file, path, "rb");
    if (!file)
    {
        std::cerr << "Error opening file" << std::endl;
        return 0;
    }

    /*Read file header*/
    fread(&fileHeader, sizeof(BITMAPFILEHEADER), 1, file);

    /*Read info file header*/
    fread(&infoHeader, sizeof(BITMAPINFOHEADER), 1, file);

    /*Check bit-depth*/
    int bytesPerPixel = 0;
    if (infoHeader.biBitCount == 32)
    {
        bytesPerPixel = 4;
    }
    else if (infoHeader.biBitCount == 24)
    {
        bytesPerPixel = 3;
    }
    else
    {
        std::cerr << "Only 24-bit and 32-bit BMPs are supported" << std::endl;
        fclose(file);
        return 0;
    }

    /*Calculate*/
    int width = infoHeader.biWidth;
    int height = abs(infoHeader.biHeight);
    bool isTopDown = (infoHeader.biHeight < 0);
    int rowByteSize = width * bytesPerPixel;
    int rowPaddedSize = (rowByteSize + 3) & ~3; // Align to 4 bytes
    unsigned char* tempRow = new unsigned char[rowPaddedSize];

    /*Read pixel data*/
    int imageSize = width * height * bytesPerPixel;
    unsigned char* data = new unsigned char[imageSize];
    fseek(file, fileHeader.bfOffBits, SEEK_SET);

    /*Read each row, padding.*/
    for (int i = 0; i < height; ++i) {
        fread(tempRow, 1, rowPaddedSize, file);
        int targetRow = isTopDown ? i : (height - 1 - i);
        memcpy(data + targetRow * rowByteSize, tempRow, rowByteSize);
    }

    fclose(file);

    /*Swap Red and Blue channels(BMP stores as BGRA or BGR)*/
    if (bytesPerPixel == 4)
    {
        for (int i = 0; i < imageSize; i += 4)
        {
            std::swap(data[i], data[i + 2]);
        }
    }
    else if (bytesPerPixel == 3)
    {
        for (int i = 0; i < imageSize; i += 3)
        {
            std::swap(data[i], data[i + 2]);
        }
    }

    /*Create OpenGL texture*/
    glBindTexture(GL_TEXTURE_2D, textureID);
    if (bytesPerPixel == 4)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    }
    else if (bytesPerPixel == 3)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenerateMipmap(GL_TEXTURE_2D);

    delete[] data;
    delete[] tempRow;

    return textureID;
}

unsigned int loadSRGBTextureFromBMP(const char* path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;

    FILE* file;
    fopen_s(&file, path, "rb");
    if (!file)
    {
        std::cerr << "Error opening file" << std::endl;
        return 0;
    }

    /*Read file header*/
    fread(&fileHeader, sizeof(BITMAPFILEHEADER), 1, file);

    /*Read info file header*/
    fread(&infoHeader, sizeof(BITMAPINFOHEADER), 1, file);

    /*Check bit-depth*/
    int bytesPerPixel = 0;
    if (infoHeader.biBitCount == 32)
    {
        bytesPerPixel = 4;
    }
    else if (infoHeader.biBitCount == 24)
    {
        bytesPerPixel = 3;
    }
    else
    {
        std::cerr << "Only 24-bit and 32-bit BMPs are supported" << std::endl;
        fclose(file);
        return 0;
    }

    /*Calculate*/
    int width = infoHeader.biWidth;
    int height = abs(infoHeader.biHeight);
    bool isTopDown = (infoHeader.biHeight < 0);
    int rowByteSize = width * bytesPerPixel;
    int rowPaddedSize = (rowByteSize + 3) & ~3; // Align to 4 bytes
    unsigned char* tempRow = new unsigned char[rowPaddedSize];

    /*Read pixel data*/
    int imageSize = width * height * bytesPerPixel;
    unsigned char* data = new unsigned char[imageSize];
    fseek(file, fileHeader.bfOffBits, SEEK_SET);

    /*Read each row, padding.*/
    for (int i = 0; i < height; ++i) {
        fread(tempRow, 1, rowPaddedSize, file);
        int targetRow = isTopDown ? i : (height - 1 - i);
        memcpy(data + targetRow * rowByteSize, tempRow, rowByteSize);
    }

    fclose(file);

    /*Swap Red and Blue channels(BMP stores as BGRA or BGR)*/
    if (bytesPerPixel == 4)
    {
        for (int i = 0; i < imageSize; i += 4)
        {
            std::swap(data[i], data[i + 2]);
        }
    }
    else if (bytesPerPixel == 3)
    {
        for (int i = 0; i < imageSize; i += 3)
        {
            std::swap(data[i], data[i + 2]);
        }
    }

    /*Create OpenGL texture*/
    glBindTexture(GL_TEXTURE_2D, textureID);
    if (bytesPerPixel == 4)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB_ALPHA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    }
    else if (bytesPerPixel == 3)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenerateMipmap(GL_TEXTURE_2D);

    delete[] data;
    delete[] tempRow;

    return textureID;
}


unsigned int loadCubemap(std::vector<std::string>& cubemapFaces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height;
    for (unsigned int i = 0; i < cubemapFaces.size(); i++)
    {
        // This will bind the texture to the textureID.
        unsigned int faceID = loadTextureFromBMP(cubemapFaces[i].c_str());

        if (faceID)
        {
            // Use glGetTexLevelParameteriv to get the width and height of the texture.
            glBindTexture(GL_TEXTURE_2D, faceID);
            glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
            glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

            // Allocate a buffer to store the texture data.
            unsigned char* data = new unsigned char[width * height * 3];  // Assuming RGB
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_SRGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

            delete[] data;
            glDeleteTextures(1, &faceID);  // Delete the intermediate texture
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << cubemapFaces[i] << std::endl;
        }
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

