#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

struct DirLight {
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
#define NR_POINT_LIGHTS 1
#define NR_SPOT_LIGHTS 4

uniform vec3 viewPos;
uniform DirLight dirLight;
uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform SpotLight spotLight[NR_SPOT_LIGHTS];

out VS_OUT{
    vec2 TexCoords;
    vec3 Normal;
    vec3 FragPos;
    mat3 TBN;
} vs_out;

out tangent_space{
    // Direcional light direction
    vec3 TangentDirlightDir;
    // Spotlight position & direction
    vec3 TangentSpotlightPos[NR_SPOT_LIGHTS];
    vec3 TangentSpotlightDir[NR_SPOT_LIGHTS];
    // Pointlight position
    vec3 TangentPointlightPos[NR_POINT_LIGHTS];

    vec3 TangentNormalDir;

    vec3 TangentViewPos;
    vec3 TangentFragPos;
} ts_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    vs_out.FragPos = vec3(model * vec4(aPos, 1.0));
    vs_out.Normal = mat3(transpose(inverse(model))) * aNormal;
    vs_out.TexCoords = aTexCoords;

    mat3 normalMatrix = transpose(inverse(mat3(model)));
    vec3 T = normalize(normalMatrix * aTangent);
    vec3 N = normalize(normalMatrix * aNormal);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);

    vs_out.TBN = mat3(T, B, N);

    mat3 TBN_inverse = transpose(vs_out.TBN);
    // Tangent directional light direction
    ts_out.TangentDirlightDir = TBN_inverse * dirLight.direction;
    // Tangent spotlight positions and directions
    for(int i = 0; i < NR_SPOT_LIGHTS; i++){
        ts_out.TangentSpotlightPos[i] = TBN_inverse * spotLight[i].position;
        ts_out.TangentSpotlightDir[i] = TBN_inverse * spotLight[i].direction;
    }

    // Tangent pointlight poistions
    for(int i = 0; i < NR_SPOT_LIGHTS; i++){
        ts_out.TangentPointlightPos[i] = TBN_inverse * pointLights[i].position;
    }

    ts_out.TangentViewPos  = TBN_inverse * viewPos;
    ts_out.TangentFragPos  = TBN_inverse * vs_out.FragPos;
    ts_out.TangentNormalDir = normalize(TBN_inverse * vs_out.Normal);

    gl_Position = projection * view * vec4(vs_out.FragPos, 1.0);
}