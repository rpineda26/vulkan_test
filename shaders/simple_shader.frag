#version 450
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPosition;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec2 fragUv;
layout(location = 4) in vec3 fragTangentPos;
layout(location = 5) in vec3 fragTangentView;
layout(location = 6) in vec3 fragTangentLightPos[10];
layout(location = 0) out vec4 outColor;

struct PointLight{
    vec4 position;
    vec4 color;
    float radius; //only used in the shader for rendering the point light
    int objId;
};
//camera view plus lighting
layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 projectionMatrix;
    mat4 viewMatrix;
    mat4 invViewMatrix;
    vec4 ambientLightColor;
    PointLight lights[10];
    int lightCount;
    int selectedLight;
    float time;
} ubo;
layout(set = 0, binding = 1) uniform sampler2D textureSampler[5]; 
layout(set = 0, binding = 2) uniform sampler2D normalSampler[5];
layout(set = 0, binding = 3) uniform sampler2D specularSampler[5];

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix;
    uint textureIndex;
    uint normalIndex;
    uint specularIndex;
    float smoothness;
    vec3 baseColor;
} push;

const float PI = 3.14159265359;
const float minimumRoughness = 0.04;
const float smoothness_input_weight = 0.75;

//distribution function
float Dist_GGX(float NdotH, float roughness) {
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float NdotH2 = NdotH * NdotH;
    float denom = NdotH2 * (alpha2 - 1.0) + 1.0;
    return alpha2 / (PI * denom * denom);
}
//geometry function
float Geometric_Shading_Smith(float NdotV, float NdotL, float roughness) {
    float alpha = roughness * roughness;
    float k = alpha / 2.0;
    float ggx1 = NdotV * sqrt(NdotL * NdotL * (1.0 - k) + k);
    float ggx2 = NdotL * sqrt(NdotV * NdotV * (1.0 - k) + k);
    return ggx1 * ggx2;
}
//fresnel function
vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

//previous model implemented: blinn phong
//this model is not updated to consider multiple point lights

void Blinn_Phong() {
    //load albedo
    vec3 texColor = texture(textureSampler[push.textureIndex], fragUv).rgb;
    
    //load normal map
    vec3 surfaceNormal = texture(normalSampler[push.normalIndex], fragUv).rgb;
    surfaceNormal = normalize(surfaceNormal * 2.0 - 1.0);

    vec3 specularLight = vec3(0.0);
    vec3 cameraPosWorld = ubo.invViewMatrix[3].xyz;
    vec3 viewDir = normalize(fragTangentView - fragTangentPos);

    vec3 directionToLight = fragTangentLightPos[0] - fragTangentPos;
    float attenuation = 1.0 / dot(directionToLight, directionToLight);
    directionToLight = normalize(directionToLight);

    //diffused light
    // vec3  lightColor = ubo.lightColor.xyz* ubo.lightColor.w *  attenuation;
    vec3 lightColor = ubo.lights[0].color.xyz * ubo.lights[0].color.w * attenuation; //temporary fix
    vec3 ambientLightColor = ubo.ambientLightColor.xyz  * ubo.ambientLightColor.w;
    float cosAngIncidence = max(dot(surfaceNormal, directionToLight), 0);
    vec3 diffuseLight = (lightColor * cosAngIncidence * texColor) + (ambientLightColor * texColor);

    //specular
    if(cosAngIncidence > 0.0) {
        vec3 halfAngle = normalize(directionToLight + viewDir);
        float blinnTerm = dot(surfaceNormal, halfAngle);
        blinnTerm = clamp(blinnTerm, 0.0, 1.0);
        blinnTerm = pow(blinnTerm, 128.0);
        specularLight += lightColor * blinnTerm;
    }
    outColor = vec4(diffuseLight * fragColor + specularLight  * texColor, 2);
}

//PBR: specular workflow
void main(){
    //load texture maps
    vec4 albedo = texture(textureSampler[push.textureIndex], fragUv); // include alpha
    albedo = albedo * vec4(fragColor, 1.0);
    vec3 surfaceNormal = texture(normalSampler[push.normalIndex], fragUv).rgb;
    vec4 specularSample = texture(specularSampler[push.specularIndex], fragUv);
    vec3 specularColor = specularSample.rgb;

    //convert smoothness to roughness
    float smoothness = mix(specularSample.a, push.smoothness, smoothness_input_weight);
    float roughness = 1 - push.smoothness * specularSample.a;
    roughness = max(roughness, minimumRoughness);


    //calculate values that does not factor in light vector
    vec3 N = normalize(surfaceNormal * 2.0 - 1.0); //convert from 0-1 to -1 to 1
    vec3 V = normalize(fragTangentView - fragTangentPos);
    float NdotV = max(dot(N, V), 0.0);

    //calculate lighting
    vec3 totalLight = vec3(0.0);
    for(int i = 0; i < ubo.lightCount; i++) {
        //define vectors
        vec3 L = normalize(fragTangentLightPos[i] - fragTangentPos);
        vec3 H = normalize(L + V);
        //dot products
        
        float NdotL = dot(N, L);
        float NdotH = max(dot(N, H), 0.0);
        float VdotH = max(dot(V, H), 0.0);

        //light attenuation
        vec3 directionToLight = fragTangentLightPos[i] - fragTangentPos;
        // float constant = 1.0;
        // float linear = 0.09;
        // float quadratic = 0.032;
        float attenuation = 1.0 / dot(directionToLight, directionToLight);
        vec3 radiance = ubo.lights[i].color.xyz * ubo.lights[i].color.w * attenuation;
        directionToLight = normalize(directionToLight);

        //specular bdrf components
        float D = Dist_GGX(NdotH, roughness);
        float G = Geometric_Shading_Smith(NdotV, NdotL, roughness);
        vec3 F0 = FresnelSchlick(VdotH, specularColor); //should come from an input value

        vec3 specular = D * G * F0 / (4 * NdotV * NdotL);

        //diffuse component
        vec3 diffuse = albedo.rgb / PI;
        totalLight += (diffuse + specular) * radiance * max(NdotL,0.0);
    }
    //ambient light
    vec3 ambient = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w * albedo.rgb;
    vec3 finalColor = totalLight + ambient;
    //tone mapping
    // finalColor = finalColor / (finalColor + vec3(1.0));
    // //gamma corrections
    // finalColor = pow(finalColor, vec3(1.0/2.2));
    outColor = vec4(finalColor, albedo.a);

}