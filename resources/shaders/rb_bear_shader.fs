#version 330 core
out vec4 FragColor;

struct Material {
    sampler2D texture_diffuse1;
    sampler2D texture_specular1;
    sampler2D texture_normal1;
    sampler2D texture_height1;

    float shininess;
};

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

in VS_OUT{
    vec2 TexCoords;
    vec3 Normal;
    vec3 FragPos;
    mat3 TBN;
} fs_in;

in tangent_space{
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
} ts_in;

uniform vec3 viewPos;
uniform DirLight dirLight;
uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform SpotLight spotLight[NR_SPOT_LIGHTS];
uniform Material material;

uniform int hasPointLight = 0;
uniform int hasSpotLight = 0;
uniform int hasDirLight = 0;

uniform int checkSpotlight[4];
uniform float transparency = 1.0;
uniform bool blinn;

uniform bool hasNormalMap = false;
uniform bool hasParallaxMapping = false;

uniform float heightScale;

vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir)
{
    // number of depth layers
    const float minLayers = 8.0;
    const float maxLayers = 32.0;
    // TODO otkriti zasto puca program kada se postavi ovo
    float numLayers = mix(maxLayers, minLayers, abs(dot(ts_in.TangentNormalDir, viewDir)));
    // float numLayers = maxLayers;
    // calculate the size of each layer
    float layerDepth = 1.0 / numLayers;
    // depth of current layer
    float currentLayerDepth = 0.0;
    // the amount to shift the texture coordinates per layer (from vector P)
    vec2 P = viewDir.xy / viewDir.z * heightScale;
    vec2 deltaTexCoords = P / numLayers;

    // get initial values
    vec2  currentTexCoords     = texCoords;
    float currentDepthMapValue = texture(material.texture_height1, currentTexCoords).r;

    while(currentLayerDepth < currentDepthMapValue)
    {
        // shift texture coordinates along direction of P
        currentTexCoords -= deltaTexCoords;
        // get depthmap value at current texture coordinates
        currentDepthMapValue = texture(material.texture_height1, currentTexCoords).r;
        // get depth of next layer
        currentLayerDepth += layerDepth;
    }

    // get texture coordinates before collision (reverse operations)
    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

    // get depth after and before collision for linear interpolation
    float afterDepth  = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = texture(material.texture_height1, prevTexCoords).r - currentLayerDepth + layerDepth;

    // interpolation of texture coordinates
    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

    return finalTexCoords;
}


vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec2 TexCoords);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec2 TexCoords);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec2 TexCoords);

// Tangent lights applied only if hasNormalMap is true
SpotLight tangentSpotlights[NR_SPOT_LIGHTS];
DirLight tangentDirlight;
PointLight tangentPointlights[NR_POINT_LIGHTS];

void main()
{
    vec3 viewDir = normalize(viewPos - fs_in.FragPos);
    vec3 tangentViewDir = normalize(ts_in.TangentViewPos - ts_in.TangentFragPos);

    vec2 texCoords;

    if(hasParallaxMapping){
        texCoords=ParallaxMapping(fs_in.TexCoords,  tangentViewDir);
    }
    else{
        texCoords = fs_in.TexCoords;
    }

    vec3 norm;
    if(!hasNormalMap){
        norm = normalize(fs_in.Normal);
    }
    else{
        norm = texture(material.texture_normal1, texCoords).rgb;
        norm = normalize(norm * 2.0 - 1.0);
    }

    vec3 result = vec3(0.0f);
    if(hasDirLight == 1){
            if(hasNormalMap){
                tangentDirlight.direction = ts_in.TangentDirlightDir;
                tangentDirlight.ambient = dirLight.ambient;
                tangentDirlight.diffuse = dirLight.diffuse;
                tangentDirlight.specular = dirLight.specular;
                result += CalcDirLight(tangentDirlight, norm, tangentViewDir, texCoords);
            }
            else{
                result += CalcDirLight(dirLight, norm, viewDir, fs_in.TexCoords);
            }
    }
    if (hasPointLight == 1){
        for (int i = 0; i < NR_POINT_LIGHTS; i++){
            if(hasNormalMap){
                tangentPointlights[i].position = ts_in.TangentPointlightPos[i];

                tangentPointlights[i].ambient = pointLights[i].ambient;
                tangentPointlights[i].diffuse = pointLights[i].diffuse;
                tangentPointlights[i].specular = pointLights[i].specular;

                tangentPointlights[i].linear = pointLights[i].linear;
                tangentPointlights[i].quadratic = pointLights[i].quadratic;
                tangentPointlights[i].constant = pointLights[i].constant;

                result += CalcPointLight(tangentPointlights[i], norm, ts_in.TangentFragPos, tangentViewDir, texCoords);
            }
            else{
                result += CalcPointLight(pointLights[i], norm, fs_in.FragPos, viewDir, fs_in.TexCoords);
            }
        }
    }
    if(hasSpotLight == 1){
        for (int i = 0; i < NR_SPOT_LIGHTS; i++){
            if (checkSpotlight[i] == 1){
                if(hasNormalMap){
                    tangentSpotlights[i].position = ts_in.TangentSpotlightPos[i];
                    tangentSpotlights[i].direction = ts_in.TangentSpotlightDir[i];

                    tangentSpotlights[i].ambient = spotLight[i].ambient;
                    tangentSpotlights[i].diffuse = spotLight[i].diffuse;
                    tangentSpotlights[i].specular = spotLight[i].specular;

                    tangentSpotlights[i].linear = spotLight[i].linear;
                    tangentSpotlights[i].quadratic = spotLight[i].quadratic;
                    tangentSpotlights[i].constant = spotLight[i].constant;

                    tangentSpotlights[i].cutOff = spotLight[i].cutOff;
                    tangentSpotlights[i].outerCutOff = spotLight[i].outerCutOff;

                    result += CalcSpotLight(tangentSpotlights[i], norm, ts_in.TangentFragPos, tangentViewDir, texCoords);
                }
                else{
                    result += CalcSpotLight(spotLight[i], norm, fs_in.FragPos, viewDir, fs_in.TexCoords);
                }
            }
        }
    }
    FragColor = vec4(result, transparency);

    //FragColor = vec4(1.0);

    //FragColor = texture(material.texture_diffuse1, fs_in.TexCoords);

}
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec2 TexCoords)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    float spec = 0.0f;
    if(blinn){
        vec3 halfwayDir = normalize(lightDir + viewDir);
        spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);
    }
    else{
        vec3 reflectDir = reflect(-lightDir, normal);
        spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    }
    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    // combine results
    vec3 ambient = light.ambient * vec3(texture(material.texture_diffuse1, TexCoords));
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.texture_diffuse1, TexCoords));
    vec3 specular = light.specular * spec * texture(material.texture_specular1, TexCoords).xxx;
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
}
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec2 TexCoords)
{
    vec3 lightDir = normalize(-light.direction);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    float spec = 0.0f;
    if(blinn){
        vec3 halfwayDir = normalize(lightDir + viewDir);
        spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);
    }
    else{
        vec3 reflectDir = reflect(-lightDir, normal);
        spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    }

    // combine results
    vec3 ambient = light.ambient * vec3(texture(material.texture_diffuse1, TexCoords));
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.texture_diffuse1, TexCoords));
    vec3 specular = light.specular * spec * texture(material.texture_specular1, TexCoords).ggg;
    return (ambient + diffuse + specular);
}

vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec2 TexCoords)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    float spec = 0.0f;
    if(blinn){
        vec3 halfwayDir = normalize(lightDir + viewDir);
        spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);
    }
    else{
        vec3 reflectDir = reflect(-lightDir, normal);
        spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    }
    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    // spotlight intensity
    float theta = dot(lightDir, normalize(-light.direction));
    //   theta = 1 treba jos malo da se istestira jer kada se stavi po defaultu, javljaju se bagovi ali deluje da lepo radi ovako
//    float theta = 1;
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

    // combine results
    vec3 ambient = light.ambient * vec3(texture(material.texture_diffuse1, TexCoords));
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.texture_diffuse1, TexCoords));
    vec3 specular = light.specular * spec * texture(material.texture_specular1, TexCoords).ggg;

    ambient *= attenuation * intensity;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;
    return (ambient + diffuse + specular);
}
