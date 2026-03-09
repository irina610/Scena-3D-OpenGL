#version 410 core

in vec3 fPosition;
in vec3 fNormal;
in vec2 fTexCoords;
in vec4 fragPosLightSpace;

out vec4 fColor;

uniform mat4 model;
uniform mat4 view;
uniform mat3 normalMatrix;

uniform vec3 dirLightDirection;
uniform vec3 dirLightColor;

uniform vec3 pointLightPosition;
uniform vec3 pointLightColor;
uniform bool pointLightOn;

uniform float constant;
uniform float linear;
uniform float quadratic;

uniform bool fogEnabled;
uniform float fogDensity;
uniform vec3 fogColor;

uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;
uniform sampler2D shadowMap; 

float ambientStrength = 0.2f;
float specularStrength = 0.5f; 
float shininess = 32.0f;

float computeFog(vec4 fPosEye, float fogDensity) {
    float fragmentDistance = length(fPosEye.xyz);
    float fogFactor = exp(-pow(fragmentDistance * fogDensity, 2));
    return clamp(fogFactor, 0.0f, 1.0f);
}

float computeShadow()
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    
    projCoords = projCoords * 0.5 + 0.5;

    // Daca e in afara hartii, nu e umbra
    if (projCoords.z > 1.0)
        return 0.0f;

    float currentDepth = projCoords.z;

    // Bias 
    vec3 normal = normalize(fNormal);
    vec3 lightDir = normalize(vec3(view * vec4(dirLightDirection, 0.0f))); 
    float bias = 0.005; 

    // PCF 
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    
    // pixelii vecini (-1, 0, +1 pe X si Y)
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
    
    return shadow;
}

void main() {
    vec4 fPosEye = view * model * vec4(fPosition, 1.0f);
    vec3 normalEye = normalize(normalMatrix * fNormal);
    vec3 viewDir = normalize(-fPosEye.xyz);
    
    vec4 texDiffuse = texture(diffuseTexture, fTexCoords);
    vec3 texSpecular = texture(specularTexture, fTexCoords).rgb;
    if (texDiffuse.a < 0.1) discard;
    
    // DIRECTIONAL LIGHT
    vec3 dirLightDirN = normalize(vec3(view * vec4(dirLightDirection, 0.0f)));
    
    // Ambient 
    vec3 ambientSun = ambientStrength * dirLightColor * texDiffuse.rgb;
    
    // Diffuse
    float diffSun = max(dot(normalEye, dirLightDirN), 0.0f);
    vec3 diffuseSun = diffSun * dirLightColor * texDiffuse.rgb;
    
    // Specular
    vec3 halfVecSun = normalize(dirLightDirN + viewDir);
    float specCoeffSun = pow(max(dot(normalEye, halfVecSun), 0.0f), shininess);
    vec3 specularSun = specularStrength * specCoeffSun * dirLightColor * texSpecular;

    // CALCUL UMBRA 
    float shadow = computeShadow();
    vec3 lightingSun = ambientSun + (1.0 - shadow) * (diffuseSun + specularSun);
    
    // POINT LIGHT
    vec3 ambientPoint = vec3(0.0);
    vec3 diffusePoint = vec3(0.0);
    vec3 specularPoint = vec3(0.0);
    
    if (pointLightOn) {
        vec3 pointLightPosEye = vec3(view * vec4(pointLightPosition, 1.0f));
        vec3 lightDir = normalize(pointLightPosEye - fPosEye.xyz);
        float distance = length(pointLightPosEye - fPosEye.xyz);
        float attenuation = 1.0f / (constant + linear * distance + quadratic * (distance * distance));
        
        ambientPoint = attenuation * ambientStrength * pointLightColor * texDiffuse.rgb;
        
        float diffP = max(dot(normalEye, lightDir), 0.0f);
        diffusePoint = attenuation * diffP * pointLightColor * texDiffuse.rgb;
        
        vec3 halfVecP = normalize(lightDir + viewDir);
        float specCoeffP = pow(max(dot(normalEye, halfVecP), 0.0f), shininess);
        specularPoint = attenuation * specularStrength * specCoeffP * pointLightColor * texSpecular;
    }
    
    vec3 finalColor = lightingSun + (ambientPoint + diffusePoint + specularPoint);
    
    // CEATA
    if (fogEnabled) {
        float fogFactor = computeFog(fPosEye, fogDensity);
        fColor = vec4(mix(fogColor, finalColor, fogFactor), texDiffuse.a);
    } else {
        fColor = vec4(finalColor, texDiffuse.a);
    }
}