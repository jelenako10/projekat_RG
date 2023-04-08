#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>


void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
unsigned int loadTexture(char const *path);
unsigned int loadCubemap(vector<std::string> &faces);
void hasLights(Shader& shader, bool directional, bool pointLight, bool spotlight);
void renderQuad();

// resolution
const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;

// camera
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct SpotLight {
    glm::vec3 position;
    glm::vec3 direction;
    float cutOff;
    float outerCutOff;

    double constant;
    double linear;
    double quadratic;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    SpotLight(){
        position = glm::vec3(0.0f, 4.0f, 0.0f);
    }
};

struct PointLight {
    glm::vec3 position;

    double constant;
    double linear;
    double quadratic;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    PointLight(){
        position = glm::vec3(-9.0f, 13.0f, 5.0f);
    }
};

struct Prozor{
    glm::vec3 position;
    float rotateX;
    float rotateY;
    float rotateZ;

    float windowScaleFactor;
};

struct DirLight {
    glm::vec3 direction;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

struct ProgramState {
    bool ImGuiEnabled = false;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;

    glm::vec3 clearColor = glm::vec3(0.0f);
    DirLight dirLight;
    SpotLight spotLight;
    PointLight pointLight;

    glm::vec3 platformPosition = glm::vec3(0.0f, 0.4321f, 0.0f);
    glm::vec3 bearPosition = glm::vec3(0.0f, 1.205f, 0.45f);
    glm::vec3 pipePosition=glm::vec3 (-13.0f, 2.0f, -13.0f);

    bool hasNormalMapping = false;
    bool hasParallaxMapping = false;

    std::vector<Prozor> prozori;
    float heightScale = 0.05;

    glm::vec3 spotlightPositions[4] = {
            glm::vec3(5.0f, 5.0f, -5.0f),
            glm::vec3(-5.0f, 5.0f, 5.0f),
            glm::vec3(-5.0f, 5.0f, -5.0f),
            glm::vec3(5.0f, 5.0f, 5.0f)
    };

    ProgramState()
        :camera(glm::vec3(0.0f, 0.0f, 0.0f)){};


    void SaveToFile(std::string filename);
    void LoadFromFile(std::string filename);
};

void initializeTransparentWindows(vector<Prozor> &prozori);

void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << clearColor.r << '\n'
        << clearColor.g << '\n'
        << clearColor.b << '\n'
        << ImGuiEnabled << '\n'
        << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z << '\n';
}

void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> clearColor.r
           >> clearColor.g
           >> clearColor.b
           >> ImGuiEnabled
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z;
    }
}

bool faceculling = false;
bool rotation1=true;
bool blinn = true;
ProgramState *programState;
Shader *shader_rb_bear;

Shader *skyShader;
bool colorSky = false;


void DrawImGui(ProgramState *programState);

int main() {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw init, create window, glad && callback functions
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    // tell GLFW to capture our mouse


    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(false);

    programState = new ProgramState;
    shader_rb_bear = new Shader("resources/shaders/rb_bear_shader.vs", "resources/shaders/rb_bear_shader.fs");
    skyShader = new Shader("resources/shaders/sky_shader.vs","resources/shaders/sky_shader.fs");

    programState->LoadFromFile("resources/program_state.txt");
    if (programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    // Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;


    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // configure global opengl state
    //2/ -----------------------------
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

    Shader skyboxShader("resources/shaders/skybox_shader.vs","resources/shaders/skybox_shader.fs");
    Shader spotlightShader("resources/shaders/spotlightShader.vs","resources/shaders/spotlightShader.fs");

    //model bear
    Model circusBear("resources/objects/circus_bear/14089_Circus_Bear_Standing_on_large_ball_v1_l2.obj");
    circusBear.SetShaderTextureNamePrefix("material.");
    unsigned int bearTextureDiffuse = loadTexture("resources/objects/circus_bear/14089_Circus_bear_standing_on_large_ball_diffuse.jpg");
    unsigned int bearTextureSpecular = loadTexture("resources/objects/circus_bear/ball_diffuse.jpg");
    unsigned int bearTextureNormal = loadTexture("resources/objects/circus_bear/cap_diffuse2.jpg");

    //model pipe
    Model pipe("resources/objects/tube/tube.obj");
    pipe.SetShaderTextureNamePrefix("material.");

    //model platform
    Model platform("resources/objects/platform/Rotating_Light_Platform_Final.fbx");
    platform.SetShaderTextureNamePrefix("material.");
    unsigned int platformTextureDiffuse = loadTexture("resources/objects/platform/lambert1_metallic.jpg");
    unsigned int platformTextureSpecular = loadTexture("resources/objects/platform/lambert1_roughness.jpg");
    unsigned int platformTextureNormal = loadTexture("resources/objects/platform/lambert1_normal.png");

    // podloga teksture
    unsigned int floorTextureDiffuse = loadTexture("resources/textures/beach_texture/Seamless_beach_sand_footsteps_texture.jpg");
    unsigned int floorTextureSpecular = loadTexture("resources/textures/beach_texture/Seamless_beach_sand_footsteps_texture_SPECULAR.jpg");
    unsigned int floorTextureNormal = loadTexture("resources/textures/beach_texture/Seamless_beach_sand_footsteps_texture_NORMAL.jpg");
    unsigned int floorTextureHeigth = loadTexture("resources/textures/beach_texture/Seamless_beach_sand_footsteps_texture_DISP.jpg");

    vector<Prozor> &prozori = programState->prozori;
    initializeTransparentWindows(prozori);

    SpotLight& spotLight = programState->spotLight;
    spotLight.direction = glm::normalize(programState->bearPosition - programState->spotLight.position);
    spotLight.ambient = glm::vec3(0.0f,0.0f,0.0f);
    spotLight.diffuse = glm::vec3(1.0f,1.0f,1.0f);
    spotLight.specular = glm::vec3(1.0f,1.0f,1.0f);
    spotLight.constant = 1.0f;
    spotLight.linear = 0.045f;
    spotLight.quadratic = 0.0005f;
    spotLight.cutOff = glm::cos(glm::radians(30.5f));
    spotLight.outerCutOff = glm::cos(glm::radians(45.0f));

    PointLight& pointLight = programState->pointLight;
    pointLight.position = programState->pointLight.position;                   // trenutno spotlight osvetljava formulu a pointlight osvetljava sve
    pointLight.ambient = glm::vec3(0.2f,0.2f,0.2f);
    pointLight.diffuse = glm::vec3(0.7f,0.7f,0.7f);
    pointLight.specular = glm::vec3(1.0f,1.0f,1.0f);
    pointLight.constant = 1.0f;
    pointLight.linear = 0.08f;
    pointLight.quadratic = 0.006f;

    DirLight& dirLight = programState->dirLight;
    dirLight.direction = glm::vec3(0.07f, 0.08f, -1.0f);
    dirLight.ambient = glm::vec3(0.08f);
    dirLight.diffuse = glm::vec3(0.3f);
    dirLight.specular = glm::vec3(0.4f);

    // cube data
    float cubeVertices[] = {
            // positions          // normals           // texture coords
            -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
            0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
            0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
            0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
            -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
            -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,

            -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
            0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
            0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
            0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
            -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
            -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

            -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
            -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
            -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
            -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
            -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
            -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

            0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
            0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
            0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
            0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
            0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
            0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

            -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
            0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
            0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
            0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,

            -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
            0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
            0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
            0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
            -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
            -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f
    };

    // lightCubeVAO with cube data
    unsigned int VBO, lightCubeVAO;
    glGenVertexArrays(1, &lightCubeVAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(lightCubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // windows transparent data
    float transparentVertices[] = {
            // positions         // texture Coords (swapped y coordinates because texture is flipped upside down)
            0.0f,  0.5f,  0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, -0.5f,  0.0f, 0.0f, 1.0f, 0.0f, 0.0f,  1.0f,
            1.0f, -0.5f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f,  1.0f,

            0.0f,  0.5f,  0.0f, 0.0f, 1.0f, 0.0f, 0.0f,  0.0f,
            1.0f, -0.5f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f,  1.0f,
            1.0f,  0.5f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f,  0.0f
    };

    // transparent VAO with transparent data
    unsigned int transparentVAO, transparentVBO;
    glGenVertexArrays(1, &transparentVAO);
    glGenBuffers(1, &transparentVBO);
    glBindVertexArray(transparentVAO);
    glBindBuffer(GL_ARRAY_BUFFER, transparentVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(transparentVertices), transparentVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    unsigned int transparentTexture = loadTexture("resources/textures/window.png");

    // skybox data
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

    // skyboxVAO with skybox data and texture loading from resources/textures/skybox/
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    vector<std::string> faces
            {
                    FileSystem::getPath("resources/textures/skybox/right.png"),
                    FileSystem::getPath("resources/textures/skybox/left.png"),
                    FileSystem::getPath("resources/textures/skybox/top.png"),
                    FileSystem::getPath("resources/textures/skybox/bottom.png"),
                    FileSystem::getPath("resources/textures/skybox/front.png"),
                    FileSystem::getPath("resources/textures/skybox/back.png")
            };

    unsigned int cubemapTexture = loadCubemap(faces);

    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);

    skyShader->use();
    skyShader->setInt("skybox", 0);

    shader_rb_bear->use();
    shader_rb_bear->setBool("blinn", blinn);

    // Brzina pomeranja na tastaturi
    programState->camera.MovementSpeed = 7.0f;

    while (!glfwWindowShouldClose(window)) {
        float currentFrame=(float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        processInput(window);

        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_CULL_FACE);
        if(faceculling)
            glCullFace(GL_FRONT);
        else
            glCullFace(GL_BACK);

        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();

        //crtanje medveda
        glm::mat4 model = glm::mat4(1.0f);
        shader_rb_bear->use();
        shader_rb_bear->setFloat("transparency", 1.0f);
        hasLights(*shader_rb_bear, true, false, true);
        shader_rb_bear->setMat4("projection", projection);
        shader_rb_bear->setMat4("view", view);

        model = glm::translate(model, programState->bearPosition);
        model = glm::scale(model, glm::vec3(0.03f, 0.03f, 0.03f));
        model= glm::rotate(model, 3.6f, glm::vec3(0.0f, 1.0f, 1.0f));
        shader_rb_bear->use();

        if(rotation1)
            model = glm::rotate(model, 0.45f* currentFrame, glm::vec3(0.0f,0.0f,1.0f));

        model = glm::translate(model, programState->bearPosition);
        shader_rb_bear->setMat4("model", model);

        shader_rb_bear->setVec3("pointLights[0].position", programState->pointLight.position);
        shader_rb_bear->setVec3("pointLights[0].ambient", programState->pointLight.ambient);
        shader_rb_bear->setVec3("pointLights[0].diffuse", programState->pointLight.diffuse);
        shader_rb_bear->setVec3("pointLights[0].specular", programState->pointLight.specular);
        shader_rb_bear->setFloat("pointLights[0].constant", programState->pointLight.constant);
        shader_rb_bear->setFloat("pointLights[0].linear", programState->pointLight.linear);
        shader_rb_bear->setFloat("pointLights[0].quadratic",programState->pointLight.quadratic);


        shader_rb_bear->setFloat("material.shininess", 32.0f);
        shader_rb_bear->setVec3("viewPos", programState->camera.Position);

        shader_rb_bear->setVec3("spotLight[1].position", programState->spotlightPositions[1]);
        spotLight.direction = glm::normalize(programState->bearPosition - programState->spotlightPositions[1]);
        shader_rb_bear->setVec3("spotLight[1].direction", programState->spotLight.direction);

        shader_rb_bear->setVec3("spotLight[1].ambient", programState->spotLight.ambient);
        shader_rb_bear->setVec3("spotLight[1].diffuse", programState->spotLight.diffuse);
        shader_rb_bear->setVec3("spotLight[1].specular", programState->spotLight.specular);
        shader_rb_bear->setFloat("spotLight[1].constant", programState->spotLight.constant);
        shader_rb_bear->setFloat("spotLight[1].linear", programState->spotLight.linear);
        shader_rb_bear->setFloat("spotLight[1].quadratic",programState->spotLight.quadratic);
        shader_rb_bear->setFloat("spotLight[1].cutOff", programState->spotLight.cutOff);
        shader_rb_bear->setFloat("spotLight[1].outerCutOff", programState->spotLight.outerCutOff);

        shader_rb_bear->setVec3("spotLight[2].position", programState->spotlightPositions[2]);
        spotLight.direction = glm::normalize(programState->bearPosition - programState->spotlightPositions[2]);
        shader_rb_bear->setVec3("spotLight[2].direction", programState->spotLight.direction);

        shader_rb_bear->setVec3("spotLight[2].ambient", programState->spotLight.ambient);
        shader_rb_bear->setVec3("spotLight[2].diffuse", programState->spotLight.diffuse);
        shader_rb_bear->setVec3("spotLight[2].specular", programState->spotLight.specular);
        shader_rb_bear->setFloat("spotLight[2].constant", programState->spotLight.constant);
        shader_rb_bear->setFloat("spotLight[2].linear", programState->spotLight.linear);
        shader_rb_bear->setFloat("spotLight[2].quadratic",programState->spotLight.quadratic);
        shader_rb_bear->setFloat("spotLight[2].cutOff", programState->spotLight.cutOff);
        shader_rb_bear->setFloat("spotLight[2].outerCutOff", programState->spotLight.outerCutOff);

        shader_rb_bear->setVec3("spotLight[3].position", programState->spotlightPositions[3]);
        spotLight.direction = glm::normalize(programState->bearPosition - programState->spotlightPositions[3]);
        shader_rb_bear->setVec3("spotLight[3].direction", programState->spotLight.direction);

        shader_rb_bear->setVec3("spotLight[3].ambient", programState->spotLight.ambient);
        shader_rb_bear->setVec3("spotLight[3].diffuse", programState->spotLight.diffuse);
        shader_rb_bear->setVec3("spotLight[3].specular", programState->spotLight.specular);
        shader_rb_bear->setFloat("spotLight[3].constant", programState->spotLight.constant);
        shader_rb_bear->setFloat("spotLight[3].linear", programState->spotLight.linear);
        shader_rb_bear->setFloat("spotLight[3].quadratic",programState->spotLight.quadratic);
        shader_rb_bear->setFloat("spotLight[3].cutOff", programState->spotLight.cutOff);
        shader_rb_bear->setFloat("spotLight[3].outerCutOff", programState->spotLight.outerCutOff);

        // directional light config
        shader_rb_bear->setVec3("dirLight.direction", dirLight.direction);
        shader_rb_bear->setVec3("dirLight.ambient", dirLight.ambient);
        shader_rb_bear->setVec3("dirLight.diffuse", dirLight.diffuse);
        shader_rb_bear->setVec3("dirLight.specular", dirLight.specular);

        shader_rb_bear->setBool("hasNormalMap", false);
        circusBear.Draw(*shader_rb_bear);

        // skyShader global config
        skyShader->use();
        skyShader->setMat4("projection", projection);
        skyShader->setMat4("view", view);
        skyShader->setVec3("cameraPos", programState->camera.Position);

        //crtanje platforme
        model = glm::mat4(1.0f);
        model = glm::translate(model, programState->platformPosition);
        model = glm::scale(model, glm::vec3(0.12f, 0.12f, 0.12f));
        model = glm::rotate(model, 0.25f , glm::vec3(0.0f, 1.0f, 0.0f));
        if(rotation1)
            model=glm::rotate(model,0.25f*currentFrame,glm::vec3(0.0f, 1.0f, 0.0f));

        if(!colorSky) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, platformTextureDiffuse);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, platformTextureSpecular);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, platformTextureNormal);

            shader_rb_bear->use();
            shader_rb_bear->setMat4("model", model);
            shader_rb_bear->setBool("hasNormalMap", programState->hasNormalMapping);
            platform.Draw(*shader_rb_bear);
            shader_rb_bear->setBool("hasNormalMap", false);
        }
        else{
            skyShader->use();
            skyShader->setMat4("model", model);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
            platform.Draw(*skyShader);
        }

        model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3(0.000000000000000001f, 0.00000000000001f, 0.00000000000000001f));
        model = glm::translate(model,programState->pipePosition);
        model= glm::rotate(model, 1.57f, glm::vec3(1.0f, 0.0f, 0.0f));
        shader_rb_bear->use();
        shader_rb_bear->setMat4("model", model);
        shader_rb_bear->setBool("hasNormalMap", programState->hasNormalMapping);
        pipe.Draw(*shader_rb_bear);
        shader_rb_bear->setBool("hasNormalMap", false);

        glDisable(GL_CULL_FACE);

        float stranica = 2.0f;
        if(!colorSky) {
            shader_rb_bear->use();
            shader_rb_bear->setInt("material.texture_height1", 3);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, floorTextureDiffuse);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, floorTextureSpecular);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, floorTextureNormal);
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, floorTextureHeigth);

            shader_rb_bear->setBool("hasNormalMap", programState->hasNormalMapping);
            shader_rb_bear->setBool("hasParallaxMapping", programState->hasParallaxMapping);
            shader_rb_bear->setFloat("heightScale", programState->heightScale);

            glm::vec3 firstPosition = glm::vec3(-25.0f * stranica, -25.0f * stranica, 0.0f);
            model = glm::translate(model, firstPosition);
            for (int i = 0; i < 50; i++) {
                for (int j = 0; j < 50; j++) {
                    model = glm::mat4(1.0f);
                    model = glm::rotate(model, glm::radians(270.0f), glm::normalize(glm::vec3(1.0f, 0.0f, 0.0f)));
                    model = glm::translate(model,
                                           firstPosition + glm::vec3((float) j * stranica, (float) i * stranica, 0.0f));
                    shader_rb_bear->setMat4("model", model);
                    renderQuad();
                }
            }
            shader_rb_bear->setBool("hasNormalMap", false);
            shader_rb_bear->setBool("hasParallaxMapping", false);
        }
        else{
            skyShader->use();
            model = glm::mat4(1.0f);
            model = glm::rotate(model, glm::radians(270.0f), glm::normalize(glm::vec3(1.0f,0.0f,0.0f)));
            model = glm::scale(model, glm::vec3(25.0f * stranica));
            skyShader->setMat4("model", model);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
            renderQuad();
        }

        // cubemap
        glDepthFunc(GL_LEQUAL);
        skyboxShader.use();
        view = glm::mat4(glm::mat3(programState->camera.GetViewMatrix()));
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);

        // skybox cube
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS);


        shader_rb_bear->use();
        shader_rb_bear->setFloat("transparency", 0.5f);

        glBindVertexArray(transparentVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, transparentTexture);

        // imgui

        if (programState->ImGuiEnabled)
            DrawImGui(programState);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    delete shader_rb_bear;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();


    {
        ImGui::Begin("Hello window");
        ImGui::Text("Hello text");
        ImGui::ColorEdit3("Background color", (float *) &programState->clearColor);

        ImGui::DragFloat("Height scale", &programState->heightScale, 0.01f, 0.0f, 1.0f);
        ImGui::End();
    }

    {
        ImGui::Begin("Camera info");
        const Camera& c = programState->camera;
        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
        ImGui::Checkbox("Camera mouse update", &programState->CameraMouseMovementUpdateEnabled);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            programState->CameraMouseMovementUpdateEnabled = true;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
    if(key == GLFW_KEY_C && action == GLFW_PRESS){
        colorSky = !colorSky;
    }
    if(key == GLFW_KEY_R && action == GLFW_PRESS) {
      rotation1=!rotation1;
    }
}
unsigned int loadTexture(char const *path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // for this tutorial: use GL_CLAMP_TO_EDGE to prevent semi-transparent borders. Due to interpolation it takes texels from next repeat
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

unsigned int loadCubemap(vector<std::string> &faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

void hasLights(Shader& shader, bool directional, bool pointLight, bool spotlight){
    //directional
    if(directional){
        shader.setInt("hasDirLight", 1);
    }
    else{
        shader.setInt("hasDirLight", 0);
    }
    // point light
    if(pointLight){
        shader.setInt("hasPointLight", 1);
    }
    else{
        shader.setInt("hasPointLight", 0);
    }
    // spotlight
    if(spotlight){
        shader.setInt("hasSpotLight", 1);
    }
    else{
        shader.setInt("hasSpotLight", 1);
    }
}

void initializeTransparentWindows(vector<Prozor> &prozori){
    Prozor p1;
    p1.position = glm::vec3(-5.5f,1.723f,5.69f);
    p1.windowScaleFactor = 1.2f;
    p1.rotateX = 0.0f;
    p1.rotateY = -20.0f;
    p1.rotateZ = 0.0f;
    prozori.push_back(p1);

    Prozor p2;
    p2.position = glm::vec3(-5.950f,1.723f,7.050f);
    p2.windowScaleFactor = 1.2f;
    p2.rotateX = 0.0f;
    p2.rotateY = -20.0f;
    p2.rotateZ = 0.0f;
    prozori.push_back(p2);

    Prozor p3;
    p3.position = glm::vec3(-6.03f,1.723f,6.900f);
    p3.windowScaleFactor = 1.2f;
    p3.rotateX = 0.0f;
    p3.rotateY = 68.5f;
    p3.rotateZ = 0.0f;
    prozori.push_back(p3);

    Prozor p4;
    p4.position = glm::vec3(-4.69f,1.723f,7.350f);
    p4.windowScaleFactor = 1.2f;
    p4.rotateX = 0.0f;
    p4.rotateY = 68.5f;
    p4.rotateZ = 0.0f;
    prozori.push_back(p4);

    Prozor p5;
    p5.position = glm::vec3(-5.8f,2.352f,6.365f);
    p5.windowScaleFactor = 1.4f;
    p5.rotateX = 90.0f;
    p5.rotateY = 0.0f;
    p5.rotateZ = 20.0f;
    prozori.push_back(p5);
}

unsigned int quadVAO = 0;
unsigned int quadVBO;

void renderQuad()
{
    if (quadVAO == 0)
    {
        // pozicije
        glm::vec3 pos1(-1.0f,  1.0f, 0.0f);
        glm::vec3 pos2(-1.0f, -1.0f, 0.0f);
        glm::vec3 pos3( 1.0f, -1.0f, 0.0f);
        glm::vec3 pos4( 1.0f,  1.0f, 0.0f);
        // texture coordinates
        glm::vec2 uv1(0.0f, 1.0f);
        glm::vec2 uv2(0.0f, 0.0f);
        glm::vec2 uv3(1.0f, 0.0f);
        glm::vec2 uv4(1.0f, 1.0f);
        // normalni vektor
        glm::vec3 nm(0.0f, 0.0f, 1.0f);

        glm::vec3 tangent1, bitangent1;
        glm::vec3 tangent2, bitangent2;
        // trougao1
        // ----------
        glm::vec3 edge1 = pos2 - pos1;
        glm::vec3 edge2 = pos3 - pos1;
        glm::vec2 deltaUV1 = uv2 - uv1;
        glm::vec2 deltaUV2 = uv3 - uv1;

        float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

        tangent1.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent1.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent1.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

        bitangent1.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
        bitangent1.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
        bitangent1.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);

        // trougao2
        // ----------
        edge1 = pos3 - pos1;
        edge2 = pos4 - pos1;
        deltaUV1 = uv3 - uv1;
        deltaUV2 = uv4 - uv1;

        f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

        tangent2.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent2.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent2.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);


        bitangent2.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
        bitangent2.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
        bitangent2.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);


        float quadVertices[] = {
                // positions            // normal         // texcoords  // tangent                          // bitangent
                pos1.x, pos1.y, pos1.z, nm.x, nm.y, nm.z, uv1.x, uv1.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,
                pos2.x, pos2.y, pos2.z, nm.x, nm.y, nm.z, uv2.x, uv2.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,
                pos3.x, pos3.y, pos3.z, nm.x, nm.y, nm.z, uv3.x, uv3.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,

                pos1.x, pos1.y, pos1.z, nm.x, nm.y, nm.z, uv1.x, uv1.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z,
                pos3.x, pos3.y, pos3.z, nm.x, nm.y, nm.z, uv3.x, uv3.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z,
                pos4.x, pos4.y, pos4.z, nm.x, nm.y, nm.z, uv4.x, uv4.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z
        };
        
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(8 * sizeof(float)));
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(11 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}
